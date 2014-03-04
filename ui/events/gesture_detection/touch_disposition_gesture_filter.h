// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_GESTURE_DETECTION_TOUCH_DISPOSITION_GESTURE_FILTER_H_
#define UI_EVENTS_GESTURE_DETECTION_TOUCH_DISPOSITION_GESTURE_FILTER_H_

#include <queue>

#include "ui/events/gesture_detection/bitset_32.h"
#include "ui/events/gesture_detection/gesture_detection_export.h"
#include "ui/events/gesture_detection/gesture_event_data_packet.h"

namespace ui {

// Interface with which the |TouchDispositionGestureFilter| forwards gestures
// for a given touch event.
class GESTURE_DETECTION_EXPORT TouchDispositionGestureFilterClient {
 public:
  virtual void ForwardGestureEvent(const GestureEventData&) = 0;
};

// Given a stream of touch-derived gesture packets, produces a refined gesture
// sequence based on the ack dispositions of the generating touch events.
class GESTURE_DETECTION_EXPORT TouchDispositionGestureFilter {
 public:
  explicit TouchDispositionGestureFilter(
      TouchDispositionGestureFilterClient* client);
  ~TouchDispositionGestureFilter();

  // To be called upon production of touch-derived gestures by the platform,
  // *prior* to the generating touch being forward to the renderer.  In
  // particular, |packet| contains [0, n] gestures that correspond to a given
  // touch event. It is imperative that a single packet is received for
  // *each* touch event, even those that did not produce a gesture.
  enum PacketResult {
    SUCCESS,              // Packet successfully queued.
    INVALID_PACKET_ORDER, // Packets were received in the wrong order, i.e.,
                          // TOUCH_BEGIN should always precede other packets.
    INVALID_PACKET_TYPE,  // Packet had an invalid type.
  };
  PacketResult OnGesturePacket(const GestureEventDataPacket& packet);

  // TODO(jdduke): Consider adoption of ui::EventResult.
  enum TouchEventAck {
    CONSUMED,
    NOT_CONSUMED,
    NO_CONSUMER_EXISTS
  };
  // To be called upon receipt of *all* touch event acks.
  void OnTouchEventAck(TouchEventAck ack_result);

  // Whether there are any active gesture sequences still queued in the filter.
  bool IsEmpty() const;

 private:
  // Utility class for tracking gesture events and dispositions for a single
  // gesture sequence. A single sequence corresponds to all gestures created
  // between the first finger down and the last finger up, including gestures
  // generated by timeouts from a statinoary finger.
  class GestureSequence {
   public:
    struct GestureHandlingState {
      GestureHandlingState();
      // True iff the sequence has had at least one touch acked.
      bool seen_ack;
      // True iff the sequence has had any touch down event consumed.
      bool start_consumed;
      // True iff the first ack received for this sequence reported that no
      // consumer exists.
      bool no_consumer;
    };

    GestureSequence();
    ~GestureSequence();

    void Push(const GestureEventDataPacket& packet);
    void Pop();
    const GestureEventDataPacket& Front() const;
    void UpdateState(GestureEventDataPacket::GestureSource gesture_source,
                     TouchEventAck ack_state);
    bool IsEmpty() const;
    const GestureHandlingState& state() const { return state_; };

   private:
    std::queue<GestureEventDataPacket> packets_;
    GestureHandlingState state_;
  };
  bool IsGesturePrevented(EventType type,
                          TouchEventAck current,
                          const GestureSequence::GestureHandlingState& state)
      const;

  void UpdateAndDispatchPackets(GestureSequence* sequence,
                                TouchEventAck ack_result);

  void FilterAndSendPacket(
      const GestureEventDataPacket& packet,
      const GestureSequence::GestureHandlingState& sequence_state,
      TouchEventAck ack_state);

  void SendGesture(const GestureEventData& gesture);
  void CancelTapIfNecessary();
  void CancelFlingIfNecessary();
  void EndScrollIfNecessary();
  GestureSequence& Head();
  GestureSequence& Tail();

  TouchDispositionGestureFilterClient* client_;
  std::queue<GestureSequence> sequences_;

  // If the previous gesture of a given type was dropped instead of being
  // dispatched, its type will occur in this set. Cleared when a new touch
  // sequence begins to be acked.
  BitSet32 last_event_of_type_dropped_;

  // Bookkeeping for inserting synthetic Gesture{Tap,Fling}Cancel events
  // when necessary, e.g., GestureTapCancel when scrolling begins, or
  // GestureFlingCancel when a user taps following a GestureFlingStart.
  bool needs_tap_ending_event_;
  bool needs_fling_ending_event_;
  bool needs_scroll_ending_event_;

  DISALLOW_COPY_AND_ASSIGN(TouchDispositionGestureFilter);
};

}  // namespace ui

#endif  // UI_EVENTS_GESTURE_DETECTION_TOUCH_DISPOSITION_GESTURE_FILTER_H_
