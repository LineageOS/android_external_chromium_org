// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_crypto_client_stream.h"

#include "net/quic/crypto/quic_decrypter.h"
#include "net/quic/crypto/quic_encrypter.h"
#include "net/quic/test_tools/quic_test_utils.h"

using base::StringPiece;
using std::vector;

namespace net {
namespace test {
namespace {

const char kServerHostname[] = "localhost";

class TestQuicVisitor : public NoOpFramerVisitor {
 public:
  TestQuicVisitor() {}

  // NoOpFramerVisitor
  virtual void OnStreamFrame(const QuicStreamFrame& frame) OVERRIDE {
    frame_ = frame;
  }

  QuicStreamFrame* frame() { return &frame_; }

 private:
  QuicStreamFrame frame_;

  DISALLOW_COPY_AND_ASSIGN(TestQuicVisitor);
};

// The same as MockHelper, except that WritePacketToWire() checks whether
// the packet has the expected contents.
class TestMockHelper : public MockHelper  {
 public:
  TestMockHelper() : packet_count_(0) {}
  virtual ~TestMockHelper() {}

  virtual int WritePacketToWire(const QuicEncryptedPacket& packet,
                                int* error) OVERRIDE {
    packet_count_++;

    // The first packet should be ClientHello.
    if (packet_count_ == 1) {
      CheckClientHelloPacket(packet);
    }

    return MockHelper::WritePacketToWire(packet, error);
  }

 private:
  void CheckClientHelloPacket(const QuicEncryptedPacket& packet);

  int packet_count_;
};

void TestMockHelper::CheckClientHelloPacket(
    const QuicEncryptedPacket& packet) {
  QuicFramer quic_framer(kQuicVersion1,
                         QuicDecrypter::Create(kNULL),
                         QuicEncrypter::Create(kNULL));
  TestQuicVisitor quic_visitor;
  quic_framer.set_visitor(&quic_visitor);
  ASSERT_TRUE(quic_framer.ProcessPacket(packet));
  EXPECT_EQ(kCryptoStreamId, quic_visitor.frame()->stream_id);
  EXPECT_FALSE(quic_visitor.frame()->fin);
  EXPECT_EQ(0u, quic_visitor.frame()->offset);

  // Check quic_visitor.frame()->data.
  scoped_ptr<CryptoHandshakeMessage> chlo(
      CryptoFramer::ParseMessage(quic_visitor.frame()->data));
  EXPECT_EQ(kCHLO, chlo->tag);

  CryptoTagValueMap& tag_value_map = chlo->tag_value_map;

  // kSNI
  EXPECT_EQ(kServerHostname, tag_value_map[kSNI]);

  // kNONC
  // TODO(wtc): check the nonce.
  ASSERT_EQ(32u, tag_value_map[kNONC].size());

  // kVERS
  ASSERT_EQ(2u, tag_value_map[kVERS].size());
  uint16 version;
  memcpy(&version, tag_value_map[kVERS].data(), 2);
  EXPECT_EQ(0u, version);

  // kKEXS
  ASSERT_EQ(4u, tag_value_map[kKEXS].size());
  CryptoTag key_exchange[1];
  memcpy(&key_exchange[0], &tag_value_map[kKEXS][0], 4);
  EXPECT_EQ(kC255, key_exchange[0]);

  // kAEAD
  ASSERT_EQ(4u, tag_value_map[kAEAD].size());
  CryptoTag cipher[1];
  memcpy(&cipher[0], &tag_value_map[kAEAD][0], 4);
  EXPECT_EQ(kAESG, cipher[0]);

  // TODO(agl): reenable these once the non-crypto parts of the handshake have
  // been split from crypto/
#if 0
  // kICSL
  ASSERT_EQ(4u, tag_value_map[kICSL].size());
  uint32 idle_lifetime;
  memcpy(&idle_lifetime, tag_value_map[kICSL].data(), 4);
  EXPECT_EQ(300u, idle_lifetime);

  // kKATO
  ASSERT_EQ(4u, tag_value_map[kKATO].size());
  uint32 keepalive_timeout;
  memcpy(&keepalive_timeout, tag_value_map[kKATO].data(), 4);
  EXPECT_EQ(0u, keepalive_timeout);

  // kCGST
  ASSERT_EQ(4u, tag_value_map[kCGST].size());
  CryptoTag congestion[1];
  memcpy(&congestion[0], &tag_value_map[kCGST][0], 4);
  EXPECT_EQ(kQBIC, congestion[0]);
#endif
}

// The same as MockSession, except that WriteData() is not mocked.
class TestMockSession : public MockSession {
 public:
  TestMockSession(QuicConnection* connection, bool is_server)
      : MockSession(connection, is_server) {
  }
  virtual ~TestMockSession() {}

  virtual QuicConsumedData WriteData(QuicStreamId id,
                                     StringPiece data,
                                     QuicStreamOffset offset,
                                     bool fin) OVERRIDE {
    return QuicSession::WriteData(id, data, offset, fin);
  }
};

class QuicCryptoClientStreamTest : public ::testing::Test {
 public:
  QuicCryptoClientStreamTest()
      : connection_(new MockConnection(1, addr_, new TestMockHelper())),
        session_(connection_, true),
        stream_(&session_, kServerHostname) {
    message_ = CreateShloMessage(&clock_, &random_, "www.google.com");
    ConstructHandshakeMessage();
  }

  void ConstructHandshakeMessage() {
    CryptoFramer framer;
    message_data_.reset(framer.ConstructHandshakeMessage(message_));
  }

  IPEndPoint addr_;
  MockConnection* connection_;
  TestMockSession session_;
  MockClock clock_;
  MockRandom random_;
  QuicCryptoClientStream stream_;
  CryptoHandshakeMessage message_;
  scoped_ptr<QuicData> message_data_;
};

TEST_F(QuicCryptoClientStreamTest, NotInitiallyConected) {
  EXPECT_FALSE(stream_.handshake_complete());
}

TEST_F(QuicCryptoClientStreamTest, ConnectedAfterSHLO) {
  stream_.CryptoConnect();
  stream_.ProcessData(message_data_->data(), message_data_->length());
  EXPECT_TRUE(stream_.handshake_complete());
}

TEST_F(QuicCryptoClientStreamTest, MessageAfterHandshake) {
  stream_.CryptoConnect();
  stream_.ProcessData(message_data_->data(), message_data_->length());

  EXPECT_CALL(*connection_, SendConnectionClose(
      QUIC_CRYPTO_MESSAGE_AFTER_HANDSHAKE_COMPLETE));
  stream_.ProcessData(message_data_->data(), message_data_->length());
}

TEST_F(QuicCryptoClientStreamTest, BadMessageType) {
  message_.tag = kCHLO;
  ConstructHandshakeMessage();

  EXPECT_CALL(*connection_,
              SendConnectionClose(QUIC_INVALID_CRYPTO_MESSAGE_TYPE));
  stream_.ProcessData(message_data_->data(), message_data_->length());
}

TEST_F(QuicCryptoClientStreamTest, CryptoConnect) {
  EXPECT_TRUE(stream_.CryptoConnect());
}

}  // namespace
}  // namespace test
}  // namespace net
