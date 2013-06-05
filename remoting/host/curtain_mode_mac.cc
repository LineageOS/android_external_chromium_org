// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/curtain_mode.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <Security/Security.h>
#include <unistd.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/single_thread_task_runner.h"
#include "remoting/host/client_session_control.h"

namespace {

using remoting::ClientSessionControl;

const char* kCGSessionPath =
    "/System/Library/CoreServices/Menu Extras/User.menu/Contents/Resources/"
    "CGSession";

// Used to detach the current session from the local console and disconnect
// the connnection if it gets re-attached.
//
// Because the switch-in handler can only called on the main (UI) thread, this
// class installs the handler and detaches the current session from the console
// on the UI thread as well.
class SessionWatcher : public base::RefCountedThreadSafe<SessionWatcher> {
 public:
  SessionWatcher(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      base::WeakPtr<ClientSessionControl> client_session_control);

  void Start();
  void Stop();

 private:
  friend class base::RefCountedThreadSafe<SessionWatcher>;
  virtual ~SessionWatcher();

  // Detaches the session from the console and install the switch-in handler to
  // detect when the session re-attaches back.
  void ActivateCurtain();

  // Installs the switch-in handler.
  bool InstallEventHandler();

  // Removes the switch-in handler.
  void RemoveEventHandler();

  // Disconnects the client session.
  void DisconnectSession();

  // Handlers for the switch-in event.
  static OSStatus SessionActivateHandler(EventHandlerCallRef handler,
                                         EventRef event,
                                         void* user_data);

  // Task runner on which public methods of this class must be called.
  scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

  // Task runner representing the thread receiving Carbon events.
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  // Used to disconnect the client session.
  base::WeakPtr<ClientSessionControl> client_session_control_;

  EventHandlerRef event_handler_;

  DISALLOW_COPY_AND_ASSIGN(SessionWatcher);
};

SessionWatcher::SessionWatcher(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control)
    : caller_task_runner_(caller_task_runner),
      ui_task_runner_(ui_task_runner),
      client_session_control_(client_session_control),
      event_handler_(NULL) {
}

void SessionWatcher::Start() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  // Activate curtain asynchronously since it has to be done on the UI thread.
  // Because the curtain activation is asynchronous, it is possible that
  // the connection will not be curtained for a brief moment. This seems to be
  // unaviodable as long as the curtain enforcement depends on processing of
  // the switch-in notifications.
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&SessionWatcher::ActivateCurtain, this));
}

void SessionWatcher::Stop() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  client_session_control_.reset();
  ui_task_runner_->PostTask(
      FROM_HERE, base::Bind(&SessionWatcher::RemoveEventHandler, this));
}

SessionWatcher::~SessionWatcher() {
  DCHECK(!event_handler_);
}

void SessionWatcher::ActivateCurtain() {
  // Curtain mode causes problems with the login screen on Lion only (starting
  // with 10.7.3), so disable it on that platform. There is a work-around, but
  // it involves modifying a system Plist pertaining to power-management, so
  // it's not something that should be done automatically. For more details,
  // see https://discussions.apple.com/thread/3209415?start=690&tstart=0
  //
  // TODO(jamiewalch): If the underlying OS bug is ever fixed, we should support
  // curtain mode on suitable versions of Lion.
  if (base::mac::IsOSLion()) {
    LOG(ERROR) << "Host curtaining is not supported on Mac OS X 10.7.";
    DisconnectSession();
    return;
  }

  // Try to install the switch-in handler. Do this before switching out the
  // current session so that the console session is not affected if it fails.
  if (!InstallEventHandler()) {
    LOG(ERROR) << "Failed to install the switch-in handler.";
    DisconnectSession();
    return;
  }

  base::mac::ScopedCFTypeRef<CFDictionaryRef> session(
      CGSessionCopyCurrentDictionary());

  // CGSessionCopyCurrentDictionary has been observed to return NULL in some
  // cases. Once the system is in this state, curtain mode will fail as the
  // CGSession command thinks the session is not attached to the console. The
  // only known remedy is logout or reboot. Since we're not sure what causes
  // this, or how common it is, a crash report is useful in this case (note
  // that the connection would have to be refused in any case, so this is no
  // loss of functionality).
  CHECK(session != NULL);

  const void* on_console = CFDictionaryGetValue(session,
                                                kCGSessionOnConsoleKey);
  const void* logged_in = CFDictionaryGetValue(session, kCGSessionLoginDoneKey);
  if (logged_in == kCFBooleanTrue && on_console == kCFBooleanTrue) {
    pid_t child = fork();
    if (child == 0) {
      execl(kCGSessionPath, kCGSessionPath, "-suspend", NULL);
      _exit(1);
    } else if (child > 0) {
      int status = 0;
      waitpid(child, &status, 0);
      if (status != 0) {
        LOG(ERROR) << kCGSessionPath << " failed.";
        DisconnectSession();
        return;
      }
    } else {
      LOG(ERROR) << "fork() failed.";
      DisconnectSession();
      return;
    }
  }
}

bool SessionWatcher::InstallEventHandler() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());
  DCHECK(!event_handler_);

  EventTypeSpec event;
  event.eventClass = kEventClassSystem;
  event.eventKind = kEventSystemUserSessionActivated;
  OSStatus result = ::InstallApplicationEventHandler(
      NewEventHandlerUPP(SessionActivateHandler), 1, &event, this,
      &event_handler_);
  if (result != noErr) {
    event_handler_ = NULL;
    DisconnectSession();
    return false;
  }

  return true;
}

void SessionWatcher::RemoveEventHandler() {
  DCHECK(ui_task_runner_->BelongsToCurrentThread());

  if (event_handler_) {
    ::RemoveEventHandler(event_handler_);
    event_handler_ = NULL;
  }
}

void SessionWatcher::DisconnectSession() {
  if (!caller_task_runner_->BelongsToCurrentThread()) {
    caller_task_runner_->PostTask(
        FROM_HERE, base::Bind(&SessionWatcher::DisconnectSession, this));
    return;
  }

  if (client_session_control_.get())
    client_session_control_->DisconnectSession();
}

OSStatus SessionWatcher::SessionActivateHandler(EventHandlerCallRef handler,
                                                EventRef event,
                                                void* user_data) {
  static_cast<SessionWatcher*>(user_data)->DisconnectSession();
  return noErr;
}

}  // namespace

namespace remoting {

class CurtainModeMac : public CurtainMode {
 public:
  CurtainModeMac(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      base::WeakPtr<ClientSessionControl> client_session_control);
  virtual ~CurtainModeMac();

  // Overriden from CurtainMode.
  virtual bool Activate() OVERRIDE;

 private:
  scoped_refptr<SessionWatcher> session_watcher_;

  DISALLOW_COPY_AND_ASSIGN(CurtainModeMac);
};

CurtainModeMac::CurtainModeMac(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control)
    : session_watcher_(new SessionWatcher(caller_task_runner,
                                          ui_task_runner,
                                          client_session_control)) {
}

CurtainModeMac::~CurtainModeMac() {
  session_watcher_->Stop();
}

bool CurtainModeMac::Activate() {
  session_watcher_->Start();
  return true;
}

// static
scoped_ptr<CurtainMode> CurtainMode::Create(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    base::WeakPtr<ClientSessionControl> client_session_control) {
  return scoped_ptr<CurtainMode>(new CurtainModeMac(caller_task_runner,
                                                    ui_task_runner,
                                                    client_session_control));
}

}  // namespace remoting
