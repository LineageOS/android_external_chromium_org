// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/ffmpeg_h264_to_annex_b_bitstream_converter.h"

#include "base/logging.h"
#include "media/ffmpeg/ffmpeg_common.h"
#include "media/formats/mp4/box_definitions.h"

namespace media {

FFmpegH264ToAnnexBBitstreamConverter::FFmpegH264ToAnnexBBitstreamConverter(
    AVCodecContext* stream_context)
    : configuration_processed_(false),
      stream_context_(stream_context) {
  CHECK(stream_context_);
}

FFmpegH264ToAnnexBBitstreamConverter::~FFmpegH264ToAnnexBBitstreamConverter() {}

bool FFmpegH264ToAnnexBBitstreamConverter::ConvertPacket(AVPacket* packet) {
  scoped_ptr<mp4::AVCDecoderConfigurationRecord> avc_config;

  if (packet == NULL || !packet->data)
    return false;

  // Calculate the needed output buffer size.
  if (!configuration_processed_) {
    if (!stream_context_->extradata || stream_context_->extradata_size <= 0)
      return false;

    avc_config.reset(new mp4::AVCDecoderConfigurationRecord());

    if (!converter_.ParseConfiguration(
            stream_context_->extradata,
            stream_context_->extradata_size,
            avc_config.get())) {
      return false;
    }
  }

  uint32 output_packet_size = converter_.CalculateNeededOutputBufferSize(
      packet->data, packet->size, avc_config.get());

  if (output_packet_size == 0)
    return false;  // Invalid input packet.

  // Allocate new packet for the output.
  AVPacket dest_packet;
  if (av_new_packet(&dest_packet, output_packet_size) != 0)
    return false;  // Memory allocation failure.

  // This is a bit tricky: since the interface does not allow us to replace
  // the pointer of the old packet with a new one, we will initially copy the
  // metadata from old packet to new bigger packet.
  dest_packet.pts = packet->pts;
  dest_packet.dts = packet->dts;
  dest_packet.pos = packet->pos;
  dest_packet.duration = packet->duration;
  dest_packet.convergence_duration = packet->convergence_duration;
  dest_packet.flags = packet->flags;
  dest_packet.stream_index = packet->stream_index;

  // Proceed with the conversion of the actual in-band NAL units, leave room
  // for configuration in the beginning.
  uint32 io_size = dest_packet.size;
  if (!converter_.ConvertNalUnitStreamToByteStream(
          packet->data, packet->size,
          avc_config.get(),
          dest_packet.data, &io_size)) {
    return false;
  }

  if (avc_config)
    configuration_processed_ = true;

  // At the end we must destroy the old packet.
  av_free_packet(packet);
  *packet = dest_packet;  // Finally, replace the values in the input packet.

  return true;
}

}  // namespace media
