// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_AUDIO_DEVICE_THREAD_H_
#define CONTENT_RENDERER_MEDIA_AUDIO_DEVICE_THREAD_H_
#pragma once

#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/shared_memory.h"
#include "base/sync_socket.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "media/audio/audio_parameters.h"

class MessageLoop;

// Data transfer between browser and render process uses a combination
// of sync sockets and shared memory. To read from the socket and render
// data, we use a worker thread, a.k.a. the AudioDeviceThread, which reads
// data from the browser via the socket and fills the shared memory from the
// audio thread via the AudioDeviceThread::Callback interface/class.
// For more details see the documentation in audio_device.h.
//
// TODO(tommi): Multiple Audio[Input]Device instances should be able to share
// the same thread instead of spinning one per instance.
class CONTENT_EXPORT AudioDeviceThread {
 public:
  // This is the callback interface/base class that Audio[Input]Device
  // implements to render input/output data. The callback happens on the
  // AudioDevice thread.
  class Callback {
   public:
    Callback(const AudioParameters& audio_parameters,
             base::SharedMemoryHandle memory,
             int memory_length);
    virtual ~Callback();

    // One time initialization for the callback object on the audio thread.
    void InitializeOnAudioThread();

    // Derived implementations must call shared_memory_.Map appropriately
    // before Process can be called.
    virtual void MapSharedMemory() = 0;

    // Called whenever we receive notifications about pending data.
    virtual void Process(int pending_data) = 0;

   protected:
    // Protected so that derived classes can access directly.
    // The variables are 'const' since values are calculated/set in the
    // constructor and must never change.
    const AudioParameters audio_parameters_;
    const int samples_per_ms_;
    const int bytes_per_ms_;

    // Audio buffers that are allocated in InitializeOnAudioThread() based on
    // info from audio_parameters_.
    std::vector<float*> audio_data_;
    base::SharedMemory shared_memory_;
    const int memory_length_;

   private:
    DISALLOW_COPY_AND_ASSIGN(Callback);
  };

  AudioDeviceThread();
  ~AudioDeviceThread();

  // Starts the audio thread. The thread must not already be running.
  void Start(AudioDeviceThread::Callback* callback,
             base::SyncSocket::Handle socket,
             const char* thread_name);

  // This tells the audio thread to stop and clean up the data.
  // The method can stop the thread synchronously or asynchronously.
  // In the latter case, the thread will still be running after Stop()
  // returns, but the callback pointer is cleared so no further callbacks will
  // be made (IOW after Stop() returns, it is safe to delete the callback).
  // The |loop_for_join| parameter is required for asynchronous operation
  // in order to join the worker thread and close the thread handle later via a
  // posted task.
  // If set to NULL, function will wait for the thread to exit before returning.
  void Stop(MessageLoop* loop_for_join);

  // Returns true if the thread is stopped or stopping.
  bool IsStopped();

 private:
  // Our own private SimpleThread override.  We implement this in a
  // private class so that we get the following benefits:
  // 1) AudioDeviceThread doesn't expose SimpleThread methods.
  //    I.e. the caller can't call Start()/Stop() - which would be bad.
  // 2) We override ThreadMain to add additional on-thread initialization
  //    while still synchronized with SimpleThread::Start() to provide
  //    reliable initialization.
  class Thread;

  base::Lock thread_lock_;
  scoped_refptr<AudioDeviceThread::Thread> thread_;

  DISALLOW_COPY_AND_ASSIGN(AudioDeviceThread);
};

#endif  // CONTENT_RENDERER_MEDIA_AUDIO_DEVICE_THREAD_H_
