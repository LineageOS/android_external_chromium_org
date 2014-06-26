// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_DECODER_SELECTOR_H_
#define MEDIA_FILTERS_DECODER_SELECTOR_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "media/base/decryptor.h"
#include "media/base/demuxer_stream.h"
#include "media/base/pipeline_status.h"
#include "media/filters/decoder_stream_traits.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class DecoderBuffer;
class DecryptingDemuxerStream;
class Decryptor;

// DecoderSelector (creates if necessary and) initializes the proper
// Decoder for a given DemuxerStream. If the given DemuxerStream is
// encrypted, a DecryptingDemuxerStream may also be created.
// The template parameter |StreamType| is the type of stream we will be
// selecting a decoder for.
template<DemuxerStream::Type StreamType>
class MEDIA_EXPORT DecoderSelector {
 public:
  typedef DecoderStreamTraits<StreamType> StreamTraits;
  typedef typename StreamTraits::DecoderType Decoder;

  // Indicates completion of Decoder selection.
  // - First parameter: The initialized Decoder. If it's set to NULL, then
  // Decoder initialization failed.
  // - Second parameter: The initialized DecryptingDemuxerStream. If it's not
  // NULL, then a DecryptingDemuxerStream is created and initialized to do
  // decryption for the initialized Decoder.
  // Note: The caller owns selected Decoder and DecryptingDemuxerStream.
  // The caller should call DecryptingDemuxerStream::Reset() before
  // calling Decoder::Reset() to release any pending decryption or read.
  typedef base::Callback<
      void(scoped_ptr<Decoder>,
           scoped_ptr<DecryptingDemuxerStream>)>
      SelectDecoderCB;

  // |decoders| contains the Decoders to use when initializing.
  //
  // |set_decryptor_ready_cb| is optional. If |set_decryptor_ready_cb| is null,
  // no decryptor will be available to perform decryption.
  DecoderSelector(
      const scoped_refptr<base::SingleThreadTaskRunner>& message_loop,
      ScopedVector<Decoder> decoders,
      const SetDecryptorReadyCB& set_decryptor_ready_cb);
  ~DecoderSelector();

  // Initializes and selects a Decoder that can decode the |stream|.
  // Selected Decoder (and DecryptingDemuxerStream) is returned via
  // the |select_decoder_cb|.
  void SelectDecoder(DemuxerStream* stream,
                     bool low_delay,
                     const SelectDecoderCB& select_decoder_cb,
                     const typename Decoder::OutputCB& output_cb);

  // Aborts pending Decoder selection and fires |select_decoder_cb| with
  // NULL and NULL immediately if it's pending.
  void Abort();

 private:
  void DecryptingDecoderInitDone(PipelineStatus status);
  void DecryptingDemuxerStreamInitDone(PipelineStatus status);
  void InitializeDecoder();
  void DecoderInitDone(PipelineStatus status);
  void ReturnNullDecoder();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  ScopedVector<Decoder> decoders_;
  SetDecryptorReadyCB set_decryptor_ready_cb_;

  DemuxerStream* input_stream_;
  bool low_delay_;
  SelectDecoderCB select_decoder_cb_;
  typename Decoder::OutputCB output_cb_;

  scoped_ptr<Decoder> decoder_;
  scoped_ptr<DecryptingDemuxerStream> decrypted_stream_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<DecoderSelector> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(DecoderSelector);
};

typedef DecoderSelector<DemuxerStream::VIDEO> VideoDecoderSelector;
typedef DecoderSelector<DemuxerStream::AUDIO> AudioDecoderSelector;

}  // namespace media

#endif  // MEDIA_FILTERS_DECODER_SELECTOR_H_
