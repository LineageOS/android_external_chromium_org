// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/sys_byteorder.h"
#include "base/time/time.h"
#include "media/cast/cast_config.h"
#include "media/cast/receiver/audio_decoder.h"
#include "media/cast/test/utility/audio_utility.h"
#include "media/cast/test/utility/standalone_cast_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/opus/src/include/opus.h"

namespace media {
namespace cast {

namespace {
struct TestScenario {
  transport::AudioCodec codec;
  int num_channels;
  int sampling_rate;

  TestScenario(transport::AudioCodec c, int n, int s)
      : codec(c), num_channels(n), sampling_rate(s) {}
};
}  // namespace

class AudioDecoderTest : public ::testing::TestWithParam<TestScenario> {
 public:
  AudioDecoderTest()
      : cast_environment_(new StandaloneCastEnvironment()),
        cond_(&lock_) {}

 protected:
  virtual void SetUp() OVERRIDE {
    audio_decoder_.reset(new AudioDecoder(cast_environment_,
                                          GetParam().num_channels,
                                          GetParam().sampling_rate,
                                          GetParam().codec));
    CHECK_EQ(STATUS_AUDIO_INITIALIZED, audio_decoder_->InitializationResult());

    audio_bus_factory_.reset(
        new TestAudioBusFactory(GetParam().num_channels,
                                GetParam().sampling_rate,
                                TestAudioBusFactory::kMiddleANoteFreq,
                                0.5f));
    last_frame_id_ = 0;
    seen_a_decoded_frame_ = false;

    if (GetParam().codec == transport::kOpus) {
      opus_encoder_memory_.reset(
          new uint8[opus_encoder_get_size(GetParam().num_channels)]);
      OpusEncoder* const opus_encoder =
          reinterpret_cast<OpusEncoder*>(opus_encoder_memory_.get());
      CHECK_EQ(OPUS_OK, opus_encoder_init(opus_encoder,
                                          GetParam().sampling_rate,
                                          GetParam().num_channels,
                                          OPUS_APPLICATION_AUDIO));
      CHECK_EQ(OPUS_OK,
               opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(OPUS_AUTO)));
    }

    total_audio_feed_in_ = base::TimeDelta();
    total_audio_decoded_ = base::TimeDelta();
  }

  // Called from the unit test thread to create another EncodedFrame and push it
  // into the decoding pipeline.
  void FeedMoreAudio(const base::TimeDelta& duration,
                     int num_dropped_frames) {
    // Prepare a simulated EncodedFrame to feed into the AudioDecoder.
    scoped_ptr<transport::EncodedFrame> encoded_frame(
        new transport::EncodedFrame());
    encoded_frame->dependency = transport::EncodedFrame::KEY;
    encoded_frame->frame_id = last_frame_id_ + 1 + num_dropped_frames;
    encoded_frame->referenced_frame_id = encoded_frame->frame_id;
    last_frame_id_ = encoded_frame->frame_id;

    const scoped_ptr<AudioBus> audio_bus(
        audio_bus_factory_->NextAudioBus(duration).Pass());

    // Encode |audio_bus| into |encoded_frame->data|.
    const int num_elements = audio_bus->channels() * audio_bus->frames();
    std::vector<int16> interleaved(num_elements);
    audio_bus->ToInterleaved(
        audio_bus->frames(), sizeof(int16), &interleaved.front());
    if (GetParam().codec == transport::kPcm16) {
      encoded_frame->data.resize(num_elements * sizeof(int16));
      int16* const pcm_data =
          reinterpret_cast<int16*>(encoded_frame->mutable_bytes());
      for (size_t i = 0; i < interleaved.size(); ++i)
        pcm_data[i] = static_cast<int16>(base::HostToNet16(interleaved[i]));
    } else if (GetParam().codec == transport::kOpus) {
      OpusEncoder* const opus_encoder =
          reinterpret_cast<OpusEncoder*>(opus_encoder_memory_.get());
      const int kOpusEncodeBufferSize = 4000;
      encoded_frame->data.resize(kOpusEncodeBufferSize);
      const int payload_size =
          opus_encode(opus_encoder,
                      &interleaved.front(),
                      audio_bus->frames(),
                      encoded_frame->mutable_bytes(),
                      encoded_frame->data.size());
      CHECK_GT(payload_size, 1);
      encoded_frame->data.resize(payload_size);
    } else {
      ASSERT_TRUE(false);  // Not reached.
    }

    {
      base::AutoLock auto_lock(lock_);
      total_audio_feed_in_ += duration;
    }

    cast_environment_->PostTask(
        CastEnvironment::MAIN,
        FROM_HERE,
        base::Bind(&AudioDecoder::DecodeFrame,
                   base::Unretained(audio_decoder_.get()),
                   base::Passed(&encoded_frame),
                   base::Bind(&AudioDecoderTest::OnDecodedFrame,
                              base::Unretained(this),
                              num_dropped_frames == 0)));
  }

  // Blocks the caller until all audio that has been feed in has been decoded.
  void WaitForAllAudioToBeDecoded() {
    DCHECK(!cast_environment_->CurrentlyOn(CastEnvironment::MAIN));
    base::AutoLock auto_lock(lock_);
    while (total_audio_decoded_ < total_audio_feed_in_)
      cond_.Wait();
    EXPECT_EQ(total_audio_feed_in_.InMicroseconds(),
              total_audio_decoded_.InMicroseconds());
  }

 private:
  // Called by |audio_decoder_| to deliver each frame of decoded audio.
  void OnDecodedFrame(bool should_be_continuous,
                      scoped_ptr<AudioBus> audio_bus,
                      bool is_continuous) {
    DCHECK(cast_environment_->CurrentlyOn(CastEnvironment::MAIN));

    // A NULL |audio_bus| indicates a decode error, which we don't expect.
    ASSERT_FALSE(!audio_bus);

    // Did the decoder detect whether frames were dropped?
    EXPECT_EQ(should_be_continuous, is_continuous);

    // Does the audio data seem to be intact?  For Opus, we have to ignore the
    // first frame seen at the start (and immediately after dropped packet
    // recovery) because it introduces a tiny, significant delay.
    bool examine_signal = true;
    if (GetParam().codec == transport::kOpus) {
      examine_signal = seen_a_decoded_frame_ && should_be_continuous;
      seen_a_decoded_frame_ = true;
    }
    if (examine_signal) {
      for (int ch = 0; ch < audio_bus->channels(); ++ch) {
        EXPECT_NEAR(
            TestAudioBusFactory::kMiddleANoteFreq * 2 * audio_bus->frames() /
                GetParam().sampling_rate,
            CountZeroCrossings(audio_bus->channel(ch), audio_bus->frames()),
            1);
      }
    }

    // Signal the main test thread that more audio was decoded.
    base::AutoLock auto_lock(lock_);
    total_audio_decoded_ += base::TimeDelta::FromSeconds(1) *
        audio_bus->frames() / GetParam().sampling_rate;
    cond_.Signal();
  }

  const scoped_refptr<StandaloneCastEnvironment> cast_environment_;
  scoped_ptr<AudioDecoder> audio_decoder_;
  scoped_ptr<TestAudioBusFactory> audio_bus_factory_;
  uint32 last_frame_id_;
  bool seen_a_decoded_frame_;
  scoped_ptr<uint8[]> opus_encoder_memory_;

  base::Lock lock_;
  base::ConditionVariable cond_;
  base::TimeDelta total_audio_feed_in_;
  base::TimeDelta total_audio_decoded_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderTest);
};

TEST_P(AudioDecoderTest, DecodesFramesWithSameDuration) {
  const base::TimeDelta kTenMilliseconds =
      base::TimeDelta::FromMilliseconds(10);
  const int kNumFrames = 10;
  for (int i = 0; i < kNumFrames; ++i)
    FeedMoreAudio(kTenMilliseconds, 0);
  WaitForAllAudioToBeDecoded();
}

TEST_P(AudioDecoderTest, DecodesFramesWithVaryingDuration) {
  // These are the set of frame durations supported by the Opus encoder.
  const int kFrameDurationMs[] = { 5, 10, 20, 40, 60 };

  const int kNumFrames = 10;
  for (size_t i = 0; i < arraysize(kFrameDurationMs); ++i)
    for (int j = 0; j < kNumFrames; ++j)
      FeedMoreAudio(base::TimeDelta::FromMilliseconds(kFrameDurationMs[i]), 0);
  WaitForAllAudioToBeDecoded();
}

TEST_P(AudioDecoderTest, RecoversFromDroppedFrames) {
  const base::TimeDelta kTenMilliseconds =
      base::TimeDelta::FromMilliseconds(10);
  const int kNumFrames = 100;
  int next_drop_at = 3;
  int next_num_dropped = 1;
  for (int i = 0; i < kNumFrames; ++i) {
    if (i == next_drop_at) {
      const int num_dropped = next_num_dropped++;
      next_drop_at *= 2;
      i += num_dropped;
      FeedMoreAudio(kTenMilliseconds, num_dropped);
    } else {
      FeedMoreAudio(kTenMilliseconds, 0);
    }
  }
  WaitForAllAudioToBeDecoded();
}

INSTANTIATE_TEST_CASE_P(AudioDecoderTestScenarios,
                        AudioDecoderTest,
                        ::testing::Values(
                             TestScenario(transport::kPcm16, 1, 8000),
                             TestScenario(transport::kPcm16, 2, 48000),
                             TestScenario(transport::kOpus, 1, 8000),
                             TestScenario(transport::kOpus, 2, 48000)));

}  // namespace cast
}  // namespace media
