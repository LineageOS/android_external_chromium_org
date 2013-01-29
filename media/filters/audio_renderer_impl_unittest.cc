// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/gtest_prod_util.h"
#include "base/message_loop.h"
#include "base/stl_util.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/base/data_buffer.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/mock_audio_renderer_sink.h"
#include "media/base/mock_filters.h"
#include "media/base/test_helpers.h"
#include "media/filters/audio_renderer_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::base::Time;
using ::base::TimeDelta;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::NiceMock;
using ::testing::StrictMock;

namespace media {

// Constants for distinguishing between muted audio and playing audio when using
// ConsumeBufferedData().
static uint8 kMutedAudio = 0x00;
static uint8 kPlayingAudio = 0x99;

class AudioRendererImplTest : public ::testing::Test {
 public:
  // Give the decoder some non-garbage media properties.
  AudioRendererImplTest()
      : renderer_(new AudioRendererImpl(new NiceMock<MockAudioRendererSink>(),
                                        SetDecryptorReadyCB())),
        demuxer_stream_(new MockDemuxerStream()),
        decoder_(new MockAudioDecoder()),
        audio_config_(kCodecVorbis, kSampleFormatPlanarF32,
                      CHANNEL_LAYOUT_STEREO, 44100, NULL, 0, false) {
    EXPECT_CALL(*demuxer_stream_, type())
        .WillRepeatedly(Return(DemuxerStream::AUDIO));
    EXPECT_CALL(*demuxer_stream_, audio_decoder_config())
        .WillRepeatedly(ReturnRef(audio_config_));

    // Stub out time.
    renderer_->set_now_cb_for_testing(base::Bind(
        &AudioRendererImplTest::GetTime, base::Unretained(this)));

    // Queue all reads from the decoder by default.
    ON_CALL(*decoder_, Read(_))
        .WillByDefault(Invoke(this, &AudioRendererImplTest::SaveReadCallback));

    // Set up audio properties.
    SetSupportedAudioDecoderProperties();
    EXPECT_CALL(*decoder_, bits_per_channel())
        .Times(AnyNumber());
    EXPECT_CALL(*decoder_, channel_layout())
        .Times(AnyNumber());
    EXPECT_CALL(*decoder_, samples_per_second())
        .Times(AnyNumber());

    decoders_.push_back(decoder_);
  }

  virtual ~AudioRendererImplTest() {
    message_loop_.RunUntilIdle();
    renderer_->Stop(NewExpectedClosure());
  }

  void SetSupportedAudioDecoderProperties() {
    ON_CALL(*decoder_, bits_per_channel())
        .WillByDefault(Return(audio_config_.bits_per_channel()));
    ON_CALL(*decoder_, channel_layout())
        .WillByDefault(Return(CHANNEL_LAYOUT_MONO));
    ON_CALL(*decoder_, samples_per_second())
        .WillByDefault(Return(audio_config_.samples_per_second()));
  }

  void SetUnsupportedAudioDecoderProperties() {
    ON_CALL(*decoder_, bits_per_channel())
        .WillByDefault(Return(3));
    ON_CALL(*decoder_, channel_layout())
        .WillByDefault(Return(CHANNEL_LAYOUT_UNSUPPORTED));
    ON_CALL(*decoder_, samples_per_second())
        .WillByDefault(Return(0));
  }

  MOCK_METHOD1(OnPrerollComplete, void(PipelineStatus));
  PipelineStatusCB NewPrerollCB() {
    return base::Bind(&AudioRendererImplTest::OnPrerollComplete,
                      base::Unretained(this));
  }

  MOCK_METHOD1(OnStatistics, void(const PipelineStatistics&));
  MOCK_METHOD0(OnUnderflow, void());
  MOCK_METHOD0(OnEnded, void());
  MOCK_METHOD0(OnDisabled, void());
  MOCK_METHOD1(OnError, void(PipelineStatus));

  void OnAudioTimeCallback(TimeDelta current_time, TimeDelta max_time) {
    CHECK(current_time <= max_time);
  }

  void Initialize() {
    EXPECT_CALL(*decoder_, Initialize(_, _, _))
        .WillOnce(RunCallback<1>(PIPELINE_OK));

    InitializeWithStatus(PIPELINE_OK);
    message_loop_.RunUntilIdle();
    int channels = ChannelLayoutToChannelCount(decoder_->channel_layout());
    int bytes_per_frame = decoder_->bits_per_channel() * channels / 8;
    next_timestamp_.reset(new AudioTimestampHelper(
        bytes_per_frame, decoder_->samples_per_second()));
  }

  void InitializeWithStatus(PipelineStatus expected) {
    renderer_->Initialize(
        demuxer_stream_,
        decoders_,
        NewExpectedStatusCB(expected),
        base::Bind(&AudioRendererImplTest::OnStatistics,
                   base::Unretained(this)),
        base::Bind(&AudioRendererImplTest::OnUnderflow,
                   base::Unretained(this)),
        base::Bind(&AudioRendererImplTest::OnAudioTimeCallback,
                   base::Unretained(this)),
        base::Bind(&AudioRendererImplTest::OnEnded,
                   base::Unretained(this)),
        base::Bind(&AudioRendererImplTest::OnDisabled,
                   base::Unretained(this)),
        base::Bind(&AudioRendererImplTest::OnError,
                   base::Unretained(this)));
  }

  void Preroll() {
    next_timestamp_->SetBaseTimestamp(TimeDelta());

    // Fill entire buffer to complete prerolling.
    EXPECT_CALL(*decoder_, Read(_));
    renderer_->Preroll(TimeDelta(), NewPrerollCB());
    EXPECT_CALL(*this, OnPrerollComplete(PIPELINE_OK));
    message_loop_.RunUntilIdle();
    DeliverRemainingAudio();
  }

  void Play() {
    renderer_->Play(NewExpectedClosure());
    renderer_->SetPlaybackRate(1.0f);
  }

  void Preroll(TimeDelta preroll_time) {
    next_timestamp_->SetBaseTimestamp(preroll_time);

    // Fill entire buffer to complete prerolling.
    EXPECT_CALL(*decoder_, Read(_));
    renderer_->Preroll(preroll_time, NewPrerollCB());
    EXPECT_CALL(*this, OnPrerollComplete(PIPELINE_OK));
    DeliverRemainingAudio();
  }

  // Delivers |size| bytes with value kPlayingAudio to |renderer_|.
  //
  // There must be a pending read callback.
  void FulfillPendingRead(size_t size) {
    CHECK(!read_cb_.is_null());
    scoped_refptr<DataBuffer> buffer(new DataBuffer(size));
    buffer->SetDataSize(size);
    memset(buffer->GetWritableData(), kPlayingAudio, buffer->GetDataSize());

    buffer->SetTimestamp(next_timestamp_->GetTimestamp());
    buffer->SetDuration(next_timestamp_->GetDuration(buffer->GetDataSize()));
    next_timestamp_->AddBytes(buffer->GetDataSize());

    base::ResetAndReturn(&read_cb_).Run(AudioDecoder::kOk, buffer);
  }

  void AbortPendingRead() {
    base::ResetAndReturn(&read_cb_).Run(AudioDecoder::kAborted, NULL);
  }

  // Delivers an end of stream buffer to |renderer_|.
  //
  // There must be a pending read callback.
  void DeliverEndOfStream() {
    CHECK(!read_cb_.is_null());
    base::ResetAndReturn(&read_cb_).Run(
        AudioDecoder::kOk, DataBuffer::CreateEOSBuffer());
  }

  // Delivers bytes until |renderer_|'s internal buffer is full and no longer
  // has pending reads.
  void DeliverRemainingAudio() {
    CHECK(!read_cb_.is_null());
    FulfillPendingRead(bytes_remaining_in_buffer());
    CHECK(read_cb_.is_null());
  }

  // Attempts to consume |size| bytes from |renderer_|'s internal buffer,
  // returning true if all |size| bytes were consumed, false if less than
  // |size| bytes were consumed.
  //
  // |muted| is optional and if passed will get set if the byte value of
  // the consumed data is muted audio.
  bool ConsumeBufferedData(uint32 size, bool* muted) {
    scoped_array<uint8> buffer(new uint8[size]);
    uint32 bytes_per_frame = (decoder_->bits_per_channel() / 8) *
        ChannelLayoutToChannelCount(decoder_->channel_layout());
    uint32 requested_frames = size / bytes_per_frame;
    uint32 frames_read = renderer_->FillBuffer(
        buffer.get(), requested_frames, 0);

    if (frames_read > 0 && muted) {
      *muted = (buffer[0] == kMutedAudio);
    }
    return (frames_read == requested_frames);
  }

  // Attempts to consume all data available from the renderer.  Returns the
  // number of frames read.  Since time is frozen, the audio delay will increase
  // as frames come in.
  int ConsumeAllBufferedData() {
    renderer_->DisableUnderflowForTesting();

    int frames_read = 0;
    int total_frames_read = 0;

    const int kRequestFrames = 1024;
    const uint32 bytes_per_frame = (decoder_->bits_per_channel() / 8) *
        ChannelLayoutToChannelCount(decoder_->channel_layout());
    scoped_array<uint8> buffer(new uint8[kRequestFrames * bytes_per_frame]);

    do {
      TimeDelta audio_delay = TimeDelta::FromMicroseconds(
          total_frames_read * Time::kMicrosecondsPerSecond /
          static_cast<float>(decoder_->samples_per_second()));

      frames_read = renderer_->FillBuffer(
          buffer.get(), kRequestFrames, audio_delay.InMilliseconds());
      total_frames_read += frames_read;
    } while (frames_read > 0);

    return total_frames_read * bytes_per_frame;
  }

  uint32 bytes_buffered() {
    return renderer_->algorithm_->bytes_buffered();
  }

  uint32 buffer_capacity() {
    return renderer_->algorithm_->QueueCapacity();
  }

  uint32 bytes_remaining_in_buffer() {
    // This can happen if too much data was delivered, in which case the buffer
    // will accept the data but not increase capacity.
    if (bytes_buffered() > buffer_capacity()) {
      return 0;
    }
    return buffer_capacity() - bytes_buffered();
  }

  void CallResumeAfterUnderflow() {
    renderer_->ResumeAfterUnderflow(false);
  }

  TimeDelta CalculatePlayTime(int bytes_filled) {
    return TimeDelta::FromMicroseconds(
        bytes_filled * Time::kMicrosecondsPerSecond /
        renderer_->audio_parameters_.GetBytesPerSecond());
  }

  void EndOfStreamTest(float playback_rate) {
    Initialize();
    Preroll();
    Play();
    renderer_->SetPlaybackRate(playback_rate);

    // Drain internal buffer, we should have a pending read.
    EXPECT_CALL(*decoder_, Read(_));
    int total_bytes = bytes_buffered();
    int bytes_filled = ConsumeAllBufferedData();

    // Due to how the cross-fade algorithm works we won't get an exact match
    // between the ideal and expected number of bytes consumed.  In the faster
    // than normal playback case, more bytes are created than should exist and
    // vice versa in the slower than normal playback case.
    const float kEpsilon = 0.10 * (total_bytes / playback_rate);
    EXPECT_NEAR(bytes_filled, total_bytes / playback_rate, kEpsilon);

    // Figure out how long until the ended event should fire.
    TimeDelta audio_play_time = CalculatePlayTime(bytes_filled);

    // Fulfill the read with an end-of-stream packet.  We shouldn't report ended
    // nor have a read until we drain the internal buffer.
    DeliverEndOfStream();

    // Advance time half way without an ended expectation.
    AdvanceTime(audio_play_time / 2);
    ConsumeBufferedData(bytes_buffered(), NULL);

    // Advance time by other half and expect the ended event.
    AdvanceTime(audio_play_time / 2);
    EXPECT_CALL(*this, OnEnded());
    ConsumeBufferedData(bytes_buffered(), NULL);
  }

  void AdvanceTime(TimeDelta time) {
    base::AutoLock auto_lock(lock_);
    time_ += time;
  }

  // Fixture members.
  scoped_refptr<AudioRendererImpl> renderer_;
  scoped_refptr<MockDemuxerStream> demuxer_stream_;
  scoped_refptr<MockAudioDecoder> decoder_;
  AudioRendererImpl::AudioDecoderList decoders_;
  AudioDecoder::ReadCB read_cb_;
  scoped_ptr<AudioTimestampHelper> next_timestamp_;
  AudioDecoderConfig audio_config_;
  MessageLoop message_loop_;

 private:
  Time GetTime() {
    base::AutoLock auto_lock(lock_);
    return time_;
  }

  void SaveReadCallback(const AudioDecoder::ReadCB& callback) {
    CHECK(read_cb_.is_null()) << "Overlapping reads are not permitted";
    read_cb_ = callback;
  }

  base::Lock lock_;
  Time time_;

  DISALLOW_COPY_AND_ASSIGN(AudioRendererImplTest);
};

TEST_F(AudioRendererImplTest, Initialize_Failed) {
  EXPECT_CALL(*decoder_, Initialize(_, _, _))
      .WillOnce(RunCallback<1>(PIPELINE_OK));
  SetUnsupportedAudioDecoderProperties();

  InitializeWithStatus(PIPELINE_ERROR_INITIALIZATION_FAILED);

  // We should have no reads.
  EXPECT_TRUE(read_cb_.is_null());
}

TEST_F(AudioRendererImplTest, Initialize_Successful) {
  Initialize();

  // We should have no reads.
  EXPECT_TRUE(read_cb_.is_null());
}

TEST_F(AudioRendererImplTest, Initialize_DecoderInitFailure) {
  EXPECT_CALL(*decoder_, Initialize(_, _, _))
      .WillOnce(RunCallback<1>(DECODER_ERROR_NOT_SUPPORTED));
  InitializeWithStatus(DECODER_ERROR_NOT_SUPPORTED);

  // We should have no reads.
  EXPECT_TRUE(read_cb_.is_null());
}

TEST_F(AudioRendererImplTest, Initialize_MultipleDecoders) {
  scoped_refptr<MockAudioDecoder> decoder1 = new MockAudioDecoder();
  // Insert |decoder1| as the first decoder in the list.
  decoders_.push_front(decoder1);
  EXPECT_CALL(*decoder1, Initialize(_, _, _))
      .WillOnce(RunCallback<1>(DECODER_ERROR_NOT_SUPPORTED));
  EXPECT_CALL(*decoder_, Initialize(_, _, _))
      .WillOnce(RunCallback<1>(PIPELINE_OK));
  InitializeWithStatus(PIPELINE_OK);

  // We should have no reads.
  EXPECT_TRUE(read_cb_.is_null());
}

TEST_F(AudioRendererImplTest, Preroll) {
  Initialize();
  Preroll();
}

TEST_F(AudioRendererImplTest, Play) {
  Initialize();
  Preroll();
  Play();

  // Drain internal buffer, we should have a pending read.
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(bytes_buffered(), NULL));
}

TEST_F(AudioRendererImplTest, EndOfStream) {
  EndOfStreamTest(1.0);
}

TEST_F(AudioRendererImplTest, EndOfStream_FasterPlaybackSpeed) {
  EndOfStreamTest(2.0);
}

TEST_F(AudioRendererImplTest, EndOfStream_SlowerPlaybackSpeed) {
  EndOfStreamTest(0.5);
}

TEST_F(AudioRendererImplTest, Underflow) {
  Initialize();
  Preroll();
  Play();

  // Drain internal buffer, we should have a pending read.
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(bytes_buffered(), NULL));

  // Verify the next FillBuffer() call triggers the underflow callback
  // since the decoder hasn't delivered any data after it was drained.
  const size_t kDataSize = 1024;
  EXPECT_CALL(*this, OnUnderflow());
  EXPECT_FALSE(ConsumeBufferedData(kDataSize, NULL));

  renderer_->ResumeAfterUnderflow(false);

  // Verify after resuming that we're still not getting data.
  //
  // NOTE: FillBuffer() satisfies the read but returns muted audio, which
  // is crazy http://crbug.com/106600
  bool muted = false;
  EXPECT_EQ(0u, bytes_buffered());
  EXPECT_TRUE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_TRUE(muted);

  // Deliver data, we should get non-muted audio.
  DeliverRemainingAudio();
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_FALSE(muted);
}

TEST_F(AudioRendererImplTest, Underflow_EndOfStream) {
  Initialize();
  Preroll();
  Play();

  // Figure out how long until the ended event should fire.  Since
  // ConsumeBufferedData() doesn't provide audio delay information, the time
  // until the ended event fires is equivalent to the longest buffered section,
  // which is the initial bytes_buffered() read.
  TimeDelta time_until_ended = CalculatePlayTime(bytes_buffered());

  // Drain internal buffer, we should have a pending read.
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(bytes_buffered(), NULL));

  // Verify the next FillBuffer() call triggers the underflow callback
  // since the decoder hasn't delivered any data after it was drained.
  const size_t kDataSize = 1024;
  EXPECT_CALL(*this, OnUnderflow());
  EXPECT_FALSE(ConsumeBufferedData(kDataSize, NULL));

  // Deliver a little bit of data.
  EXPECT_CALL(*decoder_, Read(_));
  FulfillPendingRead(kDataSize);

  // Verify we're getting muted audio during underflow.
  //
  // NOTE: FillBuffer() satisfies the read but returns muted audio, which
  // is crazy http://crbug.com/106600
  bool muted = false;
  EXPECT_EQ(kDataSize, bytes_buffered());
  EXPECT_TRUE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_TRUE(muted);

  // Now deliver end of stream, we should get our little bit of data back.
  DeliverEndOfStream();
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_EQ(kDataSize, bytes_buffered());
  EXPECT_TRUE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_FALSE(muted);

  // Deliver another end of stream buffer and attempt to read to make sure
  // we're truly at the end of stream.
  //
  // TODO(scherkus): fix AudioRendererImpl and AudioRendererAlgorithmBase to
  // stop reading after receiving an end of stream buffer. It should have also
  // fired the ended callback http://crbug.com/106641
  DeliverEndOfStream();

  AdvanceTime(time_until_ended);
  EXPECT_CALL(*this, OnEnded());
  EXPECT_FALSE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_FALSE(muted);
}

TEST_F(AudioRendererImplTest, Underflow_ResumeFromCallback) {
  Initialize();
  Preroll();
  Play();

  // Drain internal buffer, we should have a pending read.
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(bytes_buffered(), NULL));

  // Verify the next FillBuffer() call triggers the underflow callback
  // since the decoder hasn't delivered any data after it was drained.
  const size_t kDataSize = 1024;
  EXPECT_CALL(*this, OnUnderflow())
      .WillOnce(Invoke(this, &AudioRendererImplTest::CallResumeAfterUnderflow));
  EXPECT_FALSE(ConsumeBufferedData(kDataSize, NULL));

  // Verify after resuming that we're still not getting data.
  bool muted = false;
  EXPECT_EQ(0u, bytes_buffered());
  EXPECT_TRUE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_TRUE(muted);

  // Deliver data, we should get non-muted audio.
  DeliverRemainingAudio();
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(kDataSize, &muted));
  EXPECT_FALSE(muted);
}

TEST_F(AudioRendererImplTest, AbortPendingRead_Preroll) {
  Initialize();

  // Start prerolling.
  EXPECT_CALL(*decoder_, Read(_));
  renderer_->Preroll(TimeDelta(), NewPrerollCB());

  // Simulate the decoder aborting the pending read.
  EXPECT_CALL(*this, OnPrerollComplete(PIPELINE_OK));
  AbortPendingRead();

  // Preroll again to verify it completed normally.
  Preroll(TimeDelta::FromSeconds(1));

  ASSERT_TRUE(read_cb_.is_null());
}

TEST_F(AudioRendererImplTest, AbortPendingRead_Pause) {
  Initialize();

  Preroll();
  Play();

  // Partially drain internal buffer so we get a pending read.
  EXPECT_CALL(*decoder_, Read(_));
  EXPECT_TRUE(ConsumeBufferedData(bytes_buffered() / 2, NULL));

  renderer_->Pause(NewExpectedClosure());

  AbortPendingRead();

  Preroll(TimeDelta::FromSeconds(1));
}

}  // namespace media
