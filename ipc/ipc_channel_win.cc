// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/ipc_channel_win.h"

#include <windows.h>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/threading/non_thread_safe.h"
#include "base/utf_string_conversions.h"
#include "base/win/scoped_handle.h"
#include "ipc/ipc_logging.h"
#include "ipc/ipc_message_utils.h"

namespace IPC {

Channel::ChannelImpl::State::State(ChannelImpl* channel) : is_pending(false) {
  memset(&context.overlapped, 0, sizeof(context.overlapped));
  context.handler = channel;
}

Channel::ChannelImpl::State::~State() {
  COMPILE_ASSERT(!offsetof(Channel::ChannelImpl::State, context),
                 starts_with_io_context);
}

Channel::ChannelImpl::ChannelImpl(const IPC::ChannelHandle &channel_handle,
                                  Mode mode, Listener* listener)
    : ALLOW_THIS_IN_INITIALIZER_LIST(input_state_(this)),
      ALLOW_THIS_IN_INITIALIZER_LIST(output_state_(this)),
      pipe_(INVALID_HANDLE_VALUE),
      listener_(listener),
      waiting_connect_(mode & MODE_SERVER_FLAG),
      processing_incoming_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)) {
  CreatePipe(channel_handle, mode);
}

Channel::ChannelImpl::~ChannelImpl() {
  Close();
}

void Channel::ChannelImpl::Close() {
  if (thread_check_.get()) {
    DCHECK(thread_check_->CalledOnValidThread());
  }

  if (input_state_.is_pending || output_state_.is_pending)
    CancelIo(pipe_);

  // Closing the handle at this point prevents us from issuing more requests
  // form OnIOCompleted().
  if (pipe_ != INVALID_HANDLE_VALUE) {
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
  }

  // Make sure all IO has completed.
  base::Time start = base::Time::Now();
  while (input_state_.is_pending || output_state_.is_pending) {
    MessageLoopForIO::current()->WaitForIOCompletion(INFINITE, this);
  }

  while (!output_queue_.empty()) {
    Message* m = output_queue_.front();
    output_queue_.pop();
    delete m;
  }
}

bool Channel::ChannelImpl::Send(Message* message) {
  DCHECK(thread_check_->CalledOnValidThread());
  DVLOG(2) << "sending message @" << message << " on channel @" << this
           << " with type " << message->type()
           << " (" << output_queue_.size() << " in queue)";

#ifdef IPC_MESSAGE_LOG_ENABLED
  Logging::GetInstance()->OnSendMessage(message, "");
#endif

  output_queue_.push(message);
  // ensure waiting to write
  if (!waiting_connect_) {
    if (!output_state_.is_pending) {
      if (!ProcessOutgoingMessages(NULL, 0))
        return false;
    }
  }

  return true;
}

// static
bool Channel::ChannelImpl::IsNamedServerInitialized(
    const std::string& channel_id) {
  if (WaitNamedPipe(PipeName(channel_id).c_str(), 1))
    return true;
  // If ERROR_SEM_TIMEOUT occurred, the pipe exists but is handling another
  // connection.
  return GetLastError() == ERROR_SEM_TIMEOUT;
}


Channel::ChannelImpl::ReadState Channel::ChannelImpl::ReadData(
    char* buffer,
    int buffer_len,
    int* /* bytes_read */) {
  if (INVALID_HANDLE_VALUE == pipe_)
    return READ_FAILED;

  DWORD bytes_read = 0;
  BOOL ok = ReadFile(pipe_, input_buf_, Channel::kReadBufferSize,
                     &bytes_read, &input_state_.context.overlapped);
  if (!ok) {
    DWORD err = GetLastError();
    if (err == ERROR_IO_PENDING) {
      input_state_.is_pending = true;
      return READ_PENDING;
    }
    LOG(ERROR) << "pipe error: " << err;
    return READ_FAILED;
  }

  // We could return READ_SUCCEEDED here. But the way that this code is
  // structured we instead go back to the message loop. Our completion port
  // will be signalled even in the "synchronously completed" state.
  //
  // This allows us to potentially process some outgoing messages and
  // interleave other work on this thread when we're getting hammered with
  // input messages. Potentially, this could be tuned to be more efficient
  // with some testing.
  input_state_.is_pending = true;
  return READ_PENDING;
}

bool Channel::ChannelImpl::WillDispatchInputMessage(Message* msg) {
  return true;
}

void Channel::ChannelImpl::HandleHelloMessage(const Message& msg) {
  // The hello message contains one parameter containing the PID.
  listener_->OnChannelConnected(MessageIterator(msg).NextInt());
}

bool Channel::ChannelImpl::DidEmptyInputBuffers() {
  return true;
}

bool Channel::ChannelImpl::DispatchInputData(const char* input_data,
                                             int input_data_len) {
  const char* p;
  const char* end;

  // Possibly combine with the overflow buffer to make a larger buffer.
  if (input_overflow_buf_.empty()) {
    p = input_data;
    end = input_data + input_data_len;
  } else {
    if (input_overflow_buf_.size() >
        kMaximumMessageSize - input_data_len) {
      input_overflow_buf_.clear();
      LOG(ERROR) << "IPC message is too big";
      return false;
    }
    input_overflow_buf_.append(input_data, input_data_len);
    p = input_overflow_buf_.data();
    end = p + input_overflow_buf_.size();
  }

  // Dispatch all complete messages in the data buffer.
  while (p < end) {
    const char* message_tail = Message::FindNext(p, end);
    if (message_tail) {
      int len = static_cast<int>(message_tail - p);
      Message m(p, len);
      if (!WillDispatchInputMessage(&m))
        return false;

      if (IsHelloMessage(m))
        HandleHelloMessage(m);
      else
        listener_->OnMessageReceived(m);
      p = message_tail;
    } else {
      // Last message is partial.
      break;
    }
  }

  // Save any partial data in the overflow buffer.
  input_overflow_buf_.assign(p, end - p);

  if (input_overflow_buf_.empty() && !DidEmptyInputBuffers())
    return false;
  return true;
}

bool Channel::ChannelImpl::IsHelloMessage(const Message& m) const {
  return m.routing_id() == MSG_ROUTING_NONE && m.type() == HELLO_MESSAGE_TYPE;
}

bool Channel::ChannelImpl::AsyncReadComplete(int bytes_read) {
  return DispatchInputData(input_buf_, bytes_read);
}

// static
const std::wstring Channel::ChannelImpl::PipeName(
    const std::string& channel_id) {
  std::string name("\\\\.\\pipe\\chrome.");
  return ASCIIToWide(name.append(channel_id));
}

bool Channel::ChannelImpl::CreatePipe(const IPC::ChannelHandle &channel_handle,
                                      Mode mode) {
  DCHECK_EQ(INVALID_HANDLE_VALUE, pipe_);
  string16 pipe_name;
  // If we already have a valid pipe for channel just copy it.
  if (channel_handle.pipe.handle) {
    DCHECK(channel_handle.name.empty());
    pipe_name = L"Not Available";  // Just used for LOG
    // Check that the given pipe confirms to the specified mode.  We can
    // only check for PIPE_TYPE_MESSAGE & PIPE_SERVER_END flags since the
    // other flags (PIPE_TYPE_BYTE, and PIPE_CLIENT_END) are defined as 0.
    DWORD flags = 0;
    GetNamedPipeInfo(channel_handle.pipe.handle, &flags, NULL, NULL, NULL);
    DCHECK(!(flags & PIPE_TYPE_MESSAGE));
    if (((mode & MODE_SERVER_FLAG) && !(flags & PIPE_SERVER_END)) ||
        ((mode & MODE_CLIENT_FLAG) && (flags & PIPE_SERVER_END))) {
      LOG(WARNING) << "Inconsistent open mode. Mode :" << mode;
      return false;
    }
    if (!DuplicateHandle(GetCurrentProcess(),
                         channel_handle.pipe.handle,
                         GetCurrentProcess(),
                         &pipe_,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS)) {
      LOG(WARNING) << "DuplicateHandle failed. Error :" << GetLastError();
      return false;
    }
  } else if (mode & MODE_SERVER_FLAG) {
    DCHECK(!channel_handle.pipe.handle);
    const DWORD open_mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                            FILE_FLAG_FIRST_PIPE_INSTANCE;
    pipe_name = PipeName(channel_handle.name);
    pipe_ = CreateNamedPipeW(pipe_name.c_str(),
                             open_mode,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
                             1,
                             Channel::kReadBufferSize,
                             Channel::kReadBufferSize,
                             5000,
                             NULL);
  } else if (mode & MODE_CLIENT_FLAG) {
    DCHECK(!channel_handle.pipe.handle);
    pipe_name = PipeName(channel_handle.name);
    pipe_ = CreateFileW(pipe_name.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION |
                            FILE_FLAG_OVERLAPPED,
                        NULL);
  } else {
    NOTREACHED();
  }

  if (pipe_ == INVALID_HANDLE_VALUE) {
    // If this process is being closed, the pipe may be gone already.
    LOG(WARNING) << "Unable to create pipe \"" << pipe_name <<
                    "\" in " << (mode == 0 ? "server" : "client")
                    << " mode. Error :" << GetLastError();
    return false;
  }

  // Create the Hello message to be sent when Connect is called
  scoped_ptr<Message> m(new Message(MSG_ROUTING_NONE,
                                    HELLO_MESSAGE_TYPE,
                                    IPC::Message::PRIORITY_NORMAL));
  if (!m->WriteInt(GetCurrentProcessId())) {
    CloseHandle(pipe_);
    pipe_ = INVALID_HANDLE_VALUE;
    return false;
  }

  output_queue_.push(m.release());
  return true;
}

bool Channel::ChannelImpl::Connect() {
  DLOG_IF(WARNING, thread_check_.get()) << "Connect called more than once";

  if (!thread_check_.get())
    thread_check_.reset(new base::NonThreadSafe());

  if (pipe_ == INVALID_HANDLE_VALUE)
    return false;

  MessageLoopForIO::current()->RegisterIOHandler(pipe_, this);

  // Check to see if there is a client connected to our pipe...
  if (waiting_connect_)
    ProcessConnection();

  if (!input_state_.is_pending) {
    // Complete setup asynchronously. By not setting input_state_.is_pending
    // to true, we indicate to OnIOCompleted that this is the special
    // initialization signal.
    MessageLoopForIO::current()->PostTask(
        FROM_HERE, base::Bind(&Channel::ChannelImpl::OnIOCompleted,
                              weak_factory_.GetWeakPtr(), &input_state_.context,
                              0, 0));
  }

  if (!waiting_connect_)
    ProcessOutgoingMessages(NULL, 0);
  return true;
}

bool Channel::ChannelImpl::ProcessConnection() {
  DCHECK(thread_check_->CalledOnValidThread());
  if (input_state_.is_pending)
    input_state_.is_pending = false;

  // Do we have a client connected to our pipe?
  if (INVALID_HANDLE_VALUE == pipe_)
    return false;

  BOOL ok = ConnectNamedPipe(pipe_, &input_state_.context.overlapped);

  DWORD err = GetLastError();
  if (ok) {
    // Uhm, the API documentation says that this function should never
    // return success when used in overlapped mode.
    NOTREACHED();
    return false;
  }

  switch (err) {
  case ERROR_IO_PENDING:
    input_state_.is_pending = true;
    break;
  case ERROR_PIPE_CONNECTED:
    waiting_connect_ = false;
    break;
  case ERROR_NO_DATA:
    // The pipe is being closed.
    return false;
  default:
    NOTREACHED();
    return false;
  }

  return true;
}

bool Channel::ChannelImpl::ProcessIncomingMessages() {
  while (true) {
    int bytes_read = 0;
    ReadState read_state = ReadData(input_buf_, Channel::kReadBufferSize,
                                    &bytes_read);
    if (read_state == READ_FAILED)
      return false;
    if (read_state == READ_PENDING)
      return true;
    DCHECK(bytes_read > 0);
    if (!DispatchInputData(input_buf_, bytes_read))
      return false;
  }
}

bool Channel::ChannelImpl::ProcessOutgoingMessages(
    MessageLoopForIO::IOContext* context,
    DWORD bytes_written) {
  DCHECK(!waiting_connect_);  // Why are we trying to send messages if there's
                              // no connection?
  DCHECK(thread_check_->CalledOnValidThread());

  if (output_state_.is_pending) {
    DCHECK(context);
    output_state_.is_pending = false;
    if (!context || bytes_written == 0) {
      DWORD err = GetLastError();
      LOG(ERROR) << "pipe error: " << err;
      return false;
    }
    // Message was sent.
    DCHECK(!output_queue_.empty());
    Message* m = output_queue_.front();
    output_queue_.pop();
    delete m;
  }

  if (output_queue_.empty())
    return true;

  if (INVALID_HANDLE_VALUE == pipe_)
    return false;

  // Write to pipe...
  Message* m = output_queue_.front();
  DCHECK(m->size() <= INT_MAX);
  BOOL ok = WriteFile(pipe_,
                      m->data(),
                      static_cast<int>(m->size()),
                      &bytes_written,
                      &output_state_.context.overlapped);
  if (!ok) {
    DWORD err = GetLastError();
    if (err == ERROR_IO_PENDING) {
      output_state_.is_pending = true;

      DVLOG(2) << "sent pending message @" << m << " on channel @" << this
               << " with type " << m->type();

      return true;
    }
    LOG(ERROR) << "pipe error: " << err;
    return false;
  }

  DVLOG(2) << "sent message @" << m << " on channel @" << this
           << " with type " << m->type();

  output_state_.is_pending = true;
  return true;
}

void Channel::ChannelImpl::OnIOCompleted(MessageLoopForIO::IOContext* context,
                                         DWORD bytes_transfered,
                                         DWORD error) {
  bool ok = true;
  DCHECK(thread_check_->CalledOnValidThread());
  if (context == &input_state_.context) {
    if (waiting_connect_) {
      if (!ProcessConnection())
        return;
      // We may have some messages queued up to send...
      if (!output_queue_.empty() && !output_state_.is_pending)
        ProcessOutgoingMessages(NULL, 0);
      if (input_state_.is_pending)
        return;
      // else, fall-through and look for incoming messages...
    }

    // We don't support recursion through OnMessageReceived yet!
    DCHECK(!processing_incoming_);
    AutoReset<bool> auto_reset_processing_incoming(&processing_incoming_, true);

    // Process the new data.
    if (input_state_.is_pending) {
      // This is the normal case for everything except the initialization step.
      input_state_.is_pending = false;
      if (!bytes_transfered)
        ok = false;
      else
        ok = AsyncReadComplete(bytes_transfered);
    } else {
      DCHECK(!bytes_transfered);
    }

    // Request more data.
    if (ok)
      ok = ProcessIncomingMessages();
  } else {
    DCHECK(context == &output_state_.context);
    ok = ProcessOutgoingMessages(context, bytes_transfered);
  }
  if (!ok && INVALID_HANDLE_VALUE != pipe_) {
    // We don't want to re-enter Close().
    Close();
    listener_->OnChannelError();
  }
}

//------------------------------------------------------------------------------
// Channel's methods simply call through to ChannelImpl.
Channel::Channel(const IPC::ChannelHandle &channel_handle, Mode mode,
                 Listener* listener)
    : channel_impl_(new ChannelImpl(channel_handle, mode, listener)) {
}

Channel::~Channel() {
  delete channel_impl_;
}

bool Channel::Connect() {
  return channel_impl_->Connect();
}

void Channel::Close() {
  channel_impl_->Close();
}

void Channel::set_listener(Listener* listener) {
  channel_impl_->set_listener(listener);
}

bool Channel::Send(Message* message) {
  return channel_impl_->Send(message);
}

// static
bool Channel::IsNamedServerInitialized(const std::string& channel_id) {
  return ChannelImpl::IsNamedServerInitialized(channel_id);
}

}  // namespace IPC
