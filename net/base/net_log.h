// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NET_LOG_H_
#define NET_BASE_NET_LOG_H_
#pragma once

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/ref_counted.h"

class Value;

namespace base {
class TimeTicks;
}

namespace net {

// NetLog is the destination for log messages generated by the network stack.
// Each log message has a "source" field which identifies the specific entity
// that generated the message (for example, which net::URLRequest or which
// SocketStream).
//
// To avoid needing to pass in the "source id" to the logging functions, NetLog
// is usually accessed through a BoundNetLog, which will always pass in a
// specific source ID.
//
// Note that NetLog is NOT THREADSAFE.
//
// ******** The NetLog (and associated logging) is a work in progress ********
//
// TODO(eroman): Remove the 'const' qualitifer from the BoundNetLog methods.
// TODO(eroman): Make the DNS jobs emit into the NetLog.
// TODO(eroman): Start a new Source each time net::URLRequest redirects
//               (simpler to reason about each as a separate entity).

class NetLog {
 public:
  enum EventType {
#define EVENT_TYPE(label) TYPE_ ## label,
#include "net/base/net_log_event_type_list.h"
#undef EVENT_TYPE
  };

  // The 'phase' of an event trace (whether it marks the beginning or end
  // of an event.).
  enum EventPhase {
    PHASE_NONE,
    PHASE_BEGIN,
    PHASE_END,
  };

  // The "source" identifies the entity that generated the log message.
  enum SourceType {
#define SOURCE_TYPE(label, value) SOURCE_ ## label = value,
#include "net/base/net_log_source_type_list.h"
#undef SOURCE_TYPE
  };

  // Identifies the entity that generated this log. The |id| field should
  // uniquely identify the source, and is used by log observers to infer
  // message groupings. Can use NetLog::NextID() to create unique IDs.
  struct Source {
    static const uint32 kInvalidId = 0;

    Source() : type(SOURCE_NONE), id(kInvalidId) {}
    Source(SourceType type, uint32 id) : type(type), id(id) {}
    bool is_valid() const { return id != kInvalidId; }

    // The caller takes ownership of the returned Value*.
    Value* ToValue() const;

    SourceType type;
    uint32 id;
  };

  // Base class for associating additional parameters with an event. Log
  // observers need to know what specific derivations of EventParameters a
  // particular EventType uses, in order to get at the individual components.
  class EventParameters : public base::RefCountedThreadSafe<EventParameters> {
   public:
    EventParameters() {}
    virtual ~EventParameters() {}

    // Serializes the parameters to a Value tree. This is intended to be a
    // lossless conversion, which is used to serialize the parameters to JSON.
    // The caller takes ownership of the returned Value*.
    virtual Value* ToValue() const = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(EventParameters);
  };

  // Specifies the granularity of events that should be emitted to the log.
  enum LogLevel {
    // Log everything possible, even if it is slow and memory expensive.
    // Includes logging of transferred bytes.
    LOG_ALL,

    // Log all events, but do not include the actual transferred bytes as
    // parameters for bytes sent/received events.
    LOG_ALL_BUT_BYTES,

    // Only log events which are cheap, and don't consume much memory.
    LOG_BASIC,
  };

  NetLog() {}
  virtual ~NetLog() {}

  // Emits an event to the log stream.
  //  |type| - The type of the event.
  //  |time| - The time when the event occurred.
  //  |source| - The source that generated the event.
  //  |phase| - An optional parameter indicating whether this is the start/end
  //            of an action.
  //  |params| - Optional (may be NULL) parameters for this event.
  //             The specific subclass of EventParameters is defined
  //             by the contract for events of this |type|.
  //             TODO(eroman): Take a scoped_refptr<> instead.
  virtual void AddEntry(EventType type,
                        const base::TimeTicks& time,
                        const Source& source,
                        EventPhase phase,
                        EventParameters* params) = 0;

  // Returns a unique ID which can be used as a source ID.
  virtual uint32 NextID() = 0;

  // Returns the logging level for this NetLog. This is used to avoid computing
  // and saving expensive log entries.
  virtual LogLevel GetLogLevel() const = 0;

  // Converts a time to the string format that the NetLog uses to represent
  // times.  Strings are used since integers may overflow.
  static std::string TickCountToString(const base::TimeTicks& time);

  // Returns a C-String symbolic name for |event_type|.
  static const char* EventTypeToString(EventType event_type);

  // Returns a list of all the available EventTypes.
  static std::vector<EventType> GetAllEventTypes();

  // Returns a C-String symbolic name for |source_type|.
  static const char* SourceTypeToString(SourceType source_type);

  // Returns a C-String symbolic name for |event_phase|.
  static const char* EventPhaseToString(EventPhase event_phase);

  // Serializes the specified event to a DictionaryValue.
  // If |use_strings| is true, uses strings rather than numeric ids.
  static Value* EntryToDictionaryValue(net::NetLog::EventType type,
                                       const base::TimeTicks& time,
                                       const net::NetLog::Source& source,
                                       net::NetLog::EventPhase phase,
                                       net::NetLog::EventParameters* params,
                                       bool use_strings);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

// Helper that binds a Source to a NetLog, and exposes convenience methods to
// output log messages without needing to pass in the source.
class BoundNetLog {
 public:
  BoundNetLog() : net_log_(NULL) {}

  BoundNetLog(const NetLog::Source& source, NetLog* net_log)
      : source_(source), net_log_(net_log) {
  }

  // Convenience methods that call through to the NetLog, passing in the
  // currently bound source.
  void AddEntry(NetLog::EventType type,
                NetLog::EventPhase phase,
                const scoped_refptr<NetLog::EventParameters>& params) const;

  void AddEntryWithTime(
      NetLog::EventType type,
      const base::TimeTicks& time,
      NetLog::EventPhase phase,
      const scoped_refptr<NetLog::EventParameters>& params) const;

  // Convenience methods that call through to the NetLog, passing in the
  // currently bound source, current time, and a fixed "capture phase"
  // (begin, end, or none).
  void AddEvent(NetLog::EventType event_type,
                const scoped_refptr<NetLog::EventParameters>& params) const;
  void BeginEvent(NetLog::EventType event_type,
                  const scoped_refptr<NetLog::EventParameters>& params) const;
  void EndEvent(NetLog::EventType event_type,
                const scoped_refptr<NetLog::EventParameters>& params) const;

  NetLog::LogLevel GetLogLevel() const;

  // Returns true if the log level is LOG_ALL.
  bool IsLoggingBytes() const;

  // Returns true if the log level is LOG_ALL or LOG_ALL_BUT_BYTES.
  bool IsLoggingAllEvents() const;

  // Helper to create a BoundNetLog given a NetLog and a SourceType. Takes care
  // of creating a unique source ID, and handles the case of NULL net_log.
  static BoundNetLog Make(NetLog* net_log, NetLog::SourceType source_type);

  const NetLog::Source& source() const { return source_; }
  NetLog* net_log() const { return net_log_; }

 private:
  NetLog::Source source_;
  NetLog* net_log_;
};

// NetLogStringParameter is a subclass of EventParameters that encapsulates a
// single std::string parameter.
class NetLogStringParameter : public NetLog::EventParameters {
 public:
  // |name| must be a string literal.
  NetLogStringParameter(const char* name, const std::string& value);
  virtual ~NetLogStringParameter();

  const std::string& value() const {
    return value_;
  }

  virtual Value* ToValue() const;

 private:
  const char* const name_;
  const std::string value_;
};

// NetLogIntegerParameter is a subclass of EventParameters that encapsulates a
// single integer parameter.
class NetLogIntegerParameter : public NetLog::EventParameters {
 public:
  // |name| must be a string literal.
  NetLogIntegerParameter(const char* name, int value)
      : name_(name), value_(value) {}

  int value() const {
    return value_;
  }

  virtual Value* ToValue() const;

 private:
  const char* name_;
  const int value_;
};

// NetLogSourceParameter is a subclass of EventParameters that encapsulates a
// single NetLog::Source parameter.
class NetLogSourceParameter : public NetLog::EventParameters {
 public:
  // |name| must be a string literal.
  NetLogSourceParameter(const char* name, const NetLog::Source& value)
      : name_(name), value_(value) {}

  const NetLog::Source& value() const {
    return value_;
  }

  virtual Value* ToValue() const;

 private:
  const char* name_;
  const NetLog::Source value_;
};

}  // namespace net

#endif  // NET_BASE_NET_LOG_H_
