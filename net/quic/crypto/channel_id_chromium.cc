// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/crypto/channel_id_chromium.h"

#include <string>

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "crypto/ec_private_key.h"
#include "crypto/ec_signature_creator.h"
#include "net/base/net_errors.h"
#include "net/cert/asn1_util.h"
#include "net/ssl/channel_id_service.h"

namespace net {

ChannelIDKeyChromium::ChannelIDKeyChromium(
    crypto::ECPrivateKey* ec_private_key)
    : ec_private_key_(ec_private_key) {}

ChannelIDKeyChromium::~ChannelIDKeyChromium() {}

bool ChannelIDKeyChromium::Sign(base::StringPiece signed_data,
                                std::string* out_signature) const {
  scoped_ptr<crypto::ECSignatureCreator> sig_creator(
      crypto::ECSignatureCreator::Create(ec_private_key_.get()));
  if (!sig_creator) {
    return false;
  }
  const size_t len1 = strlen(ChannelIDVerifier::kContextStr) + 1;
  const size_t len2 = strlen(ChannelIDVerifier::kClientToServerStr) + 1;
  std::vector<uint8> data(len1 + len2 + signed_data.size());
  memcpy(&data[0], ChannelIDVerifier::kContextStr, len1);
  memcpy(&data[len1], ChannelIDVerifier::kClientToServerStr, len2);
  memcpy(&data[len1 + len2], signed_data.data(), signed_data.size());
  std::vector<uint8> der_signature;
  if (!sig_creator->Sign(&data[0], data.size(), &der_signature)) {
    return false;
  }
  std::vector<uint8> raw_signature;
  if (!sig_creator->DecodeSignature(der_signature, &raw_signature)) {
    return false;
  }
  memcpy(WriteInto(out_signature, raw_signature.size() + 1),
         &raw_signature[0], raw_signature.size());
  return true;
}

std::string ChannelIDKeyChromium::SerializeKey() const {
  std::string out_key;
  if (!ec_private_key_->ExportRawPublicKey(&out_key)) {
    return std::string();
  }
  return out_key;
}

// A Job handles the lookup of a single channel ID.  It is owned by the
// ChannelIDSource. If the operation can not complete synchronously, it will
// notify the ChannelIDSource upon completion.
class ChannelIDSourceChromium::Job {
 public:
  Job(ChannelIDSourceChromium* channel_id_source,
      ChannelIDService* channel_id_service);

  // Starts the channel ID lookup.  If |QUIC_PENDING| is returned, then
  // |callback| will be invoked asynchronously when the operation completes.
  QuicAsyncStatus GetChannelIDKey(const std::string& hostname,
                                  scoped_ptr<ChannelIDKey>* channel_id_key,
                                  ChannelIDSourceCallback* callback);

 private:
  enum State {
    STATE_NONE,
    STATE_GET_CHANNEL_ID_KEY,
    STATE_GET_CHANNEL_ID_KEY_COMPLETE,
  };

  int DoLoop(int last_io_result);
  void OnIOComplete(int result);
  int DoGetChannelIDKey(int result);
  int DoGetChannelIDKeyComplete(int result);

  // Channel ID source to notify when this jobs completes.
  ChannelIDSourceChromium* const channel_id_source_;

  ChannelIDService* const channel_id_service_;

  std::string channel_id_private_key_;
  std::string channel_id_cert_;
  ChannelIDService::RequestHandle channel_id_request_handle_;

  // |hostname| specifies the hostname for which we need a channel ID.
  std::string hostname_;

  scoped_ptr<ChannelIDSourceCallback> callback_;

  scoped_ptr<ChannelIDKey> channel_id_key_;

  State next_state_;

  DISALLOW_COPY_AND_ASSIGN(Job);
};

ChannelIDSourceChromium::Job::Job(
    ChannelIDSourceChromium* channel_id_source,
    ChannelIDService* channel_id_service)
    : channel_id_source_(channel_id_source),
      channel_id_service_(channel_id_service),
      next_state_(STATE_NONE) {
}

QuicAsyncStatus ChannelIDSourceChromium::Job::GetChannelIDKey(
    const std::string& hostname,
    scoped_ptr<ChannelIDKey>* channel_id_key,
    ChannelIDSourceCallback* callback) {
  DCHECK(channel_id_key);
  DCHECK(callback);

  if (STATE_NONE != next_state_) {
    DLOG(DFATAL) << "GetChannelIDKey has begun";
    return QUIC_FAILURE;
  }

  channel_id_key_.reset();

  hostname_ = hostname;

  next_state_ = STATE_GET_CHANNEL_ID_KEY;
  switch (DoLoop(OK)) {
    case OK:
      channel_id_key->reset(channel_id_key_.release());
      return QUIC_SUCCESS;
    case ERR_IO_PENDING:
      callback_.reset(callback);
      return QUIC_PENDING;
    default:
      channel_id_key->reset();
      return QUIC_FAILURE;
  }
}

int ChannelIDSourceChromium::Job::DoLoop(int last_result) {
  int rv = last_result;
  do {
    State state = next_state_;
    next_state_ = STATE_NONE;
    switch (state) {
      case STATE_GET_CHANNEL_ID_KEY:
        DCHECK(rv == OK);
        rv = DoGetChannelIDKey(rv);
        break;
      case STATE_GET_CHANNEL_ID_KEY_COMPLETE:
        rv = DoGetChannelIDKeyComplete(rv);
        break;
      case STATE_NONE:
      default:
        rv = ERR_UNEXPECTED;
        LOG(DFATAL) << "unexpected state " << state;
        break;
    }
  } while (rv != ERR_IO_PENDING && next_state_ != STATE_NONE);
  return rv;
}

void ChannelIDSourceChromium::Job::OnIOComplete(int result) {
  int rv = DoLoop(result);
  if (rv != ERR_IO_PENDING) {
    scoped_ptr<ChannelIDSourceCallback> callback(callback_.release());
    callback->Run(&channel_id_key_);
    // Will delete |this|.
    channel_id_source_->OnJobComplete(this);
  }
}

int ChannelIDSourceChromium::Job::DoGetChannelIDKey(int result) {
  next_state_ = STATE_GET_CHANNEL_ID_KEY_COMPLETE;

  return channel_id_service_->GetOrCreateChannelID(
      hostname_,
      &channel_id_private_key_,
      &channel_id_cert_,
      base::Bind(&ChannelIDSourceChromium::Job::OnIOComplete,
                 base::Unretained(this)),
      &channel_id_request_handle_);
}

int ChannelIDSourceChromium::Job::DoGetChannelIDKeyComplete(int result) {
  DCHECK_EQ(STATE_NONE, next_state_);
  if (result != OK) {
    DLOG(WARNING) << "Failed to look up channel ID: " << ErrorToString(result);
    return result;
  }

  std::vector<uint8> encrypted_private_key_info(
      channel_id_private_key_.size());
  memcpy(&encrypted_private_key_info[0], channel_id_private_key_.data(),
         channel_id_private_key_.size());

  base::StringPiece spki_piece;
  if (!asn1::ExtractSPKIFromDERCert(channel_id_cert_, &spki_piece)) {
    return ERR_UNEXPECTED;
  }
  std::vector<uint8> subject_public_key_info(spki_piece.size());
  memcpy(&subject_public_key_info[0], spki_piece.data(), spki_piece.size());

  crypto::ECPrivateKey* ec_private_key =
      crypto::ECPrivateKey::CreateFromEncryptedPrivateKeyInfo(
          ChannelIDService::kEPKIPassword, encrypted_private_key_info,
          subject_public_key_info);
  if (!ec_private_key) {
    // TODO(wtc): use the new error code ERR_CHANNEL_ID_IMPORT_FAILED to be
    // added in https://codereview.chromium.org/338093012/.
    return ERR_UNEXPECTED;
  }
  channel_id_key_.reset(new ChannelIDKeyChromium(ec_private_key));

  return result;
}

ChannelIDSourceChromium::ChannelIDSourceChromium(
    ChannelIDService* channel_id_service)
    : channel_id_service_(channel_id_service) {
}

ChannelIDSourceChromium::~ChannelIDSourceChromium() {
  STLDeleteElements(&active_jobs_);
}

QuicAsyncStatus ChannelIDSourceChromium::GetChannelIDKey(
    const std::string& hostname,
    scoped_ptr<ChannelIDKey>* channel_id_key,
    ChannelIDSourceCallback* callback) {
  scoped_ptr<Job> job(new Job(this, channel_id_service_));
  QuicAsyncStatus status = job->GetChannelIDKey(hostname, channel_id_key,
                                                callback);
  if (status == QUIC_PENDING) {
    active_jobs_.insert(job.release());
  }
  return status;
}

void ChannelIDSourceChromium::OnJobComplete(Job* job) {
  active_jobs_.erase(job);
  delete job;
}

}  // namespace net
