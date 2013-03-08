// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Fix rate send side congestion control, used for testing.

#ifndef NET_QUIC_CONGESTION_CONTROL_FIX_RATE_SENDER_H_
#define NET_QUIC_CONGESTION_CONTROL_FIX_RATE_SENDER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/base/net_export.h"
#include "net/quic/quic_clock.h"
#include "net/quic/quic_time.h"
#include "net/quic/congestion_control/leaky_bucket.h"
#include "net/quic/congestion_control/paced_sender.h"
#include "net/quic/congestion_control/send_algorithm_interface.h"

namespace net {

class NET_EXPORT_PRIVATE FixRateSender : public SendAlgorithmInterface {

 public:
  explicit FixRateSender(const QuicClock* clock);

  // Start implementation of SendAlgorithmInterface.
  virtual void OnIncomingQuicCongestionFeedbackFrame(
      const QuicCongestionFeedbackFrame& feedback,
      QuicTime feedback_receive_time,
      QuicBandwidth sent_bandwidth,
      const SentPacketsMap& sent_packets) OVERRIDE;
  virtual void OnIncomingAck(QuicPacketSequenceNumber acked_sequence_number,
                             QuicByteCount acked_bytes,
                             QuicTime::Delta rtt) OVERRIDE;
  virtual void OnIncomingLoss(QuicTime ack_receive_time) OVERRIDE;
  virtual void SentPacket(QuicTime sent_time,
                          QuicPacketSequenceNumber equence_number,
                          QuicByteCount bytes,
                          bool is_retransmission,
                          bool has_retransmittable_data) OVERRIDE;
  virtual QuicTime::Delta TimeUntilSend(QuicTime now,
                                        bool is_retransmission) OVERRIDE;
  virtual QuicBandwidth BandwidthEstimate() OVERRIDE;
  // End implementation of SendAlgorithmInterface.

 private:
  QuicByteCount CongestionWindow();

  QuicBandwidth bitrate_;
  LeakyBucket fix_rate_leaky_bucket_;
  PacedSender paced_sender_;
  QuicByteCount data_in_flight_;

  DISALLOW_COPY_AND_ASSIGN(FixRateSender);
};

}  // namespace net

#endif  // NET_QUIC_CONGESTION_CONTROL_FIX_RATE_SENDER_H_
