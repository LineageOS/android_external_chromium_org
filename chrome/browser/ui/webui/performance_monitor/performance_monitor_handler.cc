// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/performance_monitor/performance_monitor_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/time.h"
#include "base/values.h"
#include "chrome/browser/performance_monitor/database.h"
#include "chrome/browser/performance_monitor/event.h"
#include "chrome/browser/performance_monitor/metric.h"
#include "chrome/browser/performance_monitor/performance_monitor.h"
#include "chrome/browser/performance_monitor/performance_monitor_util.h"
#include "chrome/browser/ui/webui/performance_monitor/performance_monitor_l10n.h"
#include "chrome/browser/ui/webui/performance_monitor/performance_monitor_ui_constants.h"
#include "chrome/browser/ui/webui/performance_monitor/performance_monitor_ui_util.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/value_builder.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"

using content::BrowserThread;

namespace performance_monitor {
namespace {

std::set<MetricType> GetMetricSetForCategory(MetricCategory category) {
  std::set<MetricType> metric_set;
  switch (category) {
    case METRIC_CATEGORY_CPU:
      metric_set.insert(METRIC_CPU_USAGE);
      break;
    case METRIC_CATEGORY_MEMORY:
      metric_set.insert(METRIC_SHARED_MEMORY_USAGE);
      metric_set.insert(METRIC_PRIVATE_MEMORY_USAGE);
      break;
    case METRIC_CATEGORY_TIMING:
      metric_set.insert(METRIC_STARTUP_TIME);
      metric_set.insert(METRIC_TEST_STARTUP_TIME);
      metric_set.insert(METRIC_SESSION_RESTORE_TIME);
      metric_set.insert(METRIC_PAGE_LOAD_TIME);
      break;
    case METRIC_CATEGORY_NETWORK:
      metric_set.insert(METRIC_NETWORK_BYTES_READ);
      break;
    default:
      NOTREACHED();
  }
  return metric_set;
}

std::set<EventType> GetEventSetForCategory(EventCategory category) {
  std::set<EventType> event_set;
  switch (category) {
    case EVENT_CATEGORY_EXTENSIONS:
      event_set.insert(EVENT_EXTENSION_INSTALL);
      event_set.insert(EVENT_EXTENSION_UNINSTALL);
      event_set.insert(EVENT_EXTENSION_UPDATE);
      event_set.insert(EVENT_EXTENSION_ENABLE);
      event_set.insert(EVENT_EXTENSION_DISABLE);
      break;
    case EVENT_CATEGORY_CHROME:
      event_set.insert(EVENT_CHROME_UPDATE);
      break;
    case EVENT_CATEGORY_EXCEPTIONS:
      event_set.insert(EVENT_RENDERER_HANG);
      event_set.insert(EVENT_RENDERER_CRASH);
      event_set.insert(EVENT_RENDERER_KILLED);
      event_set.insert(EVENT_UNCLEAN_EXIT);
      break;
    default:
      NOTREACHED();
  }
  return event_set;
}

Unit GetUnitForMetricCategory(MetricCategory category) {
  switch (category) {
    case METRIC_CATEGORY_CPU:
      return UNIT_PERCENT;
    case METRIC_CATEGORY_MEMORY:
      return UNIT_MEGABYTES;
    case METRIC_CATEGORY_TIMING:
      return UNIT_SECONDS;
    case METRIC_CATEGORY_NETWORK:
      return UNIT_MEGABYTES;
    default:
      NOTREACHED();
  }
  return UNIT_UNDEFINED;
}

MetricCategory GetCategoryForMetric(MetricType type) {
  switch (type) {
    case METRIC_CPU_USAGE:
      return METRIC_CATEGORY_CPU;
    case METRIC_SHARED_MEMORY_USAGE:
    case METRIC_PRIVATE_MEMORY_USAGE:
      return METRIC_CATEGORY_MEMORY;
    case METRIC_STARTUP_TIME:
    case METRIC_TEST_STARTUP_TIME:
    case METRIC_SESSION_RESTORE_TIME:
    case METRIC_PAGE_LOAD_TIME:
      return METRIC_CATEGORY_TIMING;
    case METRIC_NETWORK_BYTES_READ:
      return METRIC_CATEGORY_NETWORK;
    default:
      NOTREACHED();
  }
  return METRIC_CATEGORY_NUMBER_OF_CATEGORIES;
}

Unit GetUnitForMetricType(MetricType type) {
  switch (type) {
    case METRIC_CPU_USAGE:
      return UNIT_PERCENT;
    case METRIC_SHARED_MEMORY_USAGE:
    case METRIC_PRIVATE_MEMORY_USAGE:
    case METRIC_NETWORK_BYTES_READ:
      return UNIT_BYTES;
    case METRIC_STARTUP_TIME:
    case METRIC_TEST_STARTUP_TIME:
    case METRIC_SESSION_RESTORE_TIME:
    case METRIC_PAGE_LOAD_TIME:
      return UNIT_MICROSECONDS;
    default:
      NOTREACHED();
  }
  return UNIT_UNDEFINED;
}

// Returns a dictionary for the aggregation method. Aggregation strategies
// contain an id representing the method, and localized strings for the
// method name and method description.
scoped_ptr<DictionaryValue> GetAggregationMethod(
    AggregationMethod method) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  value->SetInteger("id", method);
  value->SetString("name", GetLocalizedStringFromAggregationMethod(method));
  value->SetString(
      "description",
      GetLocalizedStringForAggregationMethodDescription(method));
  return value.Pass();
}

// Returns a list of metric details, with one entry per metric. Metric details
// are dictionaries which contain the id representing the metric and localized
// strings for the metric name and metric description.
scoped_ptr<ListValue> GetMetricDetailsForCategory(MetricCategory category) {
  scoped_ptr<ListValue> value(new ListValue());
  std::set<MetricType> metric_set = GetMetricSetForCategory(category);
  for (std::set<MetricType>::const_iterator iter = metric_set.begin();
       iter != metric_set.end(); ++iter) {
    DictionaryValue* metric_details = new DictionaryValue();
    metric_details->SetInteger("metricId", *iter);
    metric_details->SetString(
        "name", GetLocalizedStringFromMetricType(*iter));
    metric_details->SetString(
        "description", GetLocalizedStringForMetricTypeDescription(*iter));
    value->Append(metric_details);
  }
  return value.Pass();
}

// Returns a dictionary for the metric category. Metric categories contain
// an id representing the category; localized strings for the category name,
// the default unit in which the category is measured, and the category's
// description; and the metric details for each metric type in the category.
scoped_ptr<DictionaryValue> GetMetricCategory(MetricCategory category) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  value->SetInteger("metricCategoryId", category);
  value->SetString(
      "name", GetLocalizedStringFromMetricCategory(category));
  value->SetString(
      "unit",
      GetLocalizedStringFromUnit(GetUnitForMetricCategory(category)));
  value->SetString(
      "description",
      GetLocalizedStringForMetricCategoryDescription(category));
  value->Set("details", GetMetricDetailsForCategory(category).release());
  return value.Pass();
}

// Returns a list of event types, with one entry per event. Event types
// are dictionaries which contain the id representing the event and localized
// strings for the event name, event description, and a title suitable for a
// mouseover popup.
scoped_ptr<ListValue> GetEventTypesForCategory(EventCategory category) {
  scoped_ptr<ListValue> value(new ListValue());
  std::set<EventType> event_set = GetEventSetForCategory(category);
  for (std::set<EventType>::const_iterator iter = event_set.begin();
       iter != event_set.end(); ++iter) {
    DictionaryValue* event_details = new DictionaryValue();
    event_details->SetInteger("eventId", *iter);
    event_details->SetString(
        "name", GetLocalizedStringFromEventType(*iter));
    event_details->SetString(
        "description", GetLocalizedStringForEventTypeDescription(*iter));
    event_details->SetString(
        "popupTitle", GetLocalizedStringForEventTypeMouseover(*iter));
    value->Append(event_details);
  }
  return value.Pass();
}

// Returns a dictionary for the event category. Event categories contain an
// id representing the category, localized strings for the event name and
// event description, and event details for each event type in the category.
scoped_ptr<DictionaryValue> GetEventCategory(EventCategory category) {
  scoped_ptr<DictionaryValue> value(new DictionaryValue());
  value->SetInteger("eventCategoryId", category);
  value->SetString(
      "name", GetLocalizedStringFromEventCategory(category));
  value->SetString(
      "description",
      GetLocalizedStringForEventCategoryDescription(category));
  value->Set("details", GetEventTypesForCategory(category).release());
  return value.Pass();
}

// Queries the performance monitor database for active intervals between
// |start| and |end| times and appends the results to |results|.
void DoGetActiveIntervals(ListValue* results,
                          const base::Time& start, const base::Time& end) {
  Database* db = PerformanceMonitor::GetInstance()->database();
  std::vector<TimeRange> intervals = db->GetActiveIntervals(start, end);

  for (std::vector<TimeRange>::iterator it = intervals.begin();
       it != intervals.end(); ++it) {
    DictionaryValue* interval_value = new DictionaryValue();
    interval_value->SetDouble("start", it->start.ToJsTime());
    interval_value->SetDouble("end", it->end.ToJsTime());
    results->Append(interval_value);
  }
}

// Queries the PerformanceMonitor database for events of type |event_type|
// between |start| and |end| times, creates a new event with localized keys
// for display, and appends the results to |results|.
void DoGetEvents(ListValue* results, std::set<EventType> event_types,
                 const base::Time& start, const base::Time& end) {
  Database* db = PerformanceMonitor::GetInstance()->database();

  for (std::set<EventType>::const_iterator iter = event_types.begin();
       iter != event_types.end(); ++iter) {
    DictionaryValue* event_results = new DictionaryValue();
    event_results->SetInteger("eventId", static_cast<int>(*iter));
    ListValue* events = new ListValue();
    event_results->Set("events", events);
    results->Append(event_results);

    Database::EventVector event_vector = db->GetEvents(*iter, start, end);

    for (Database::EventVector::iterator event = event_vector.begin();
         event != event_vector.end(); ++event) {
      DictionaryValue* localized_event = new DictionaryValue();

      for (DictionaryValue::key_iterator key = (*event)->data()->begin_keys();
           key != (*event)->data()->end_keys(); ++key) {
        std::string localized_key;

        Value* value = NULL;

        // The property 'eventType' is set in HandleGetEvents as part of the
        // entire result set, so we don't need to include this here in the
        // event.
        if (*key == "eventType")
          continue;
        else if (*key == "time") {
          // The property 'time' is also used computationally, but must be
          // converted to JS-style time.
          double time = 0.0;
          if (!(*event)->data()->GetDouble(std::string("time"), &time)) {
            LOG(ERROR) << "Failed to get 'time' field from event.";
            continue;
          }
          value = Value::CreateDoubleValue(
              base::Time::FromInternalValue(
                  static_cast<int64>(time)).ToJsTime());
        } else {
          // All other values are user-facing, so we create a new value for
          // localized display.
          DictionaryValue* localized_value = new DictionaryValue();
          localized_value->SetString("label",
                                     GetLocalizedStringFromEventProperty(*key));
          Value* old_value = NULL;
          (*event)->data()->Get(*key, &old_value);
          localized_value->SetWithoutPathExpansion("value",
                                                   old_value->DeepCopy());
          value = localized_value;
        }

        localized_event->SetWithoutPathExpansion(*key, value);
      }
      events->Append(localized_event);
    }
  }
}

// Create a list of metric data for a given time range and metric. The data
// will be aggregated according to |aggregation_method|, and times will be
// converted to JS-style times. This data will be passed to the UI for display.
scoped_ptr<ListValue> GetDisplayMetricsForInterval(
    const TimeRange& interval,
    MetricType metric_type,
    const base::TimeDelta& resolution,
    AggregationMethod aggregation_method,
    double conversion_factor) {
  Database* db = PerformanceMonitor::GetInstance()->database();
  scoped_ptr<Database::MetricVector> metric_vector =
      db->GetStatsForActivityAndMetric(
          metric_type, interval.start, interval.end);
  scoped_ptr<Database::MetricVector> aggregated_metrics =
      AggregateMetric(metric_type,
                      metric_vector.get(),
                      interval.start,
                      resolution,
                      aggregation_method);

  scoped_ptr<ListValue> metrics_list(new ListValue());

  if (!aggregated_metrics)
    return metrics_list.Pass();

  for (Database::MetricVector::const_iterator iter =
           aggregated_metrics->begin();
       iter != aggregated_metrics->end(); ++iter) {
    DictionaryValue* metric_value = new DictionaryValue();
    metric_value->SetDouble("time", iter->time.ToJsTime());
    metric_value->SetDouble("value", iter->value * conversion_factor);
    metrics_list->Append(metric_value);
  }

  return metrics_list.Pass();
}

// Populates results with a dictionary for each metric requested. The dictionary
// includes a metric id, the maximum value for the metric, and a list of lists
// of metric points, with each sublist containing the data for an interval
// for which PerformanceMonitor was active.
void DoGetMetrics(ListValue* results,
                  const std::set<MetricType> metric_types,
                  const base::Time& start,
                  const base::Time& end,
                  const base::TimeDelta& resolution,
                  AggregationMethod aggregation_method) {
  Database* db = PerformanceMonitor::GetInstance()->database();
  std::vector<TimeRange> intervals = db->GetActiveIntervals(start, end);

  // For each metric type, populate a new dictionary and append it to results.
  for (std::set<MetricType>::const_iterator metric_type = metric_types.begin();
       metric_type != metric_types.end(); ++metric_type) {
    double conversion_factor =
        GetConversionFactor(*GetUnitDetails(GetUnitForMetricType(*metric_type)),
                            *GetUnitDetails(GetUnitForMetricCategory(
                                GetCategoryForMetric(*metric_type))));

    DictionaryValue* metric_set = new DictionaryValue();
    metric_set->SetInteger("metricId", static_cast<int>(*metric_type));
    metric_set->SetDouble(
        "maxValue",
        db->GetMaxStatsForActivityAndMetric(*metric_type) * conversion_factor);

    ListValue* metric_points_by_interval = new ListValue();
    for (std::vector<TimeRange>::const_iterator time_range = intervals.begin();
         time_range != intervals.end(); ++time_range) {
      metric_points_by_interval->Append(
          GetDisplayMetricsForInterval(*time_range,
                                       *metric_type,
                                       resolution,
                                       aggregation_method,
                                       conversion_factor).release());
    }

    metric_set->Set("metrics", metric_points_by_interval);

    results->Append(metric_set);
  }
}

}  // namespace

PerformanceMonitorHandler::PerformanceMonitorHandler() {
  // If we are not running the --run-performance-monitor flag, we will not have
  // started PerformanceMonitor.
  if (!PerformanceMonitor::initialized())
    PerformanceMonitor::GetInstance()->Start();
}

PerformanceMonitorHandler::~PerformanceMonitorHandler() {}

void PerformanceMonitorHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getActiveIntervals",
      base::Bind(&PerformanceMonitorHandler::HandleGetActiveIntervals,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "getFlagEnabled",
      base::Bind(&PerformanceMonitorHandler::HandleGetFlagEnabled,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "getAggregationTypes",
      base::Bind(&PerformanceMonitorHandler::HandleGetAggregationTypes,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "getEventTypes",
      base::Bind(&PerformanceMonitorHandler::HandleGetEventTypes,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "getEvents",
      base::Bind(&PerformanceMonitorHandler::HandleGetEvents,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "getMetricTypes",
      base::Bind(&PerformanceMonitorHandler::HandleGetMetricTypes,
                 AsWeakPtr()));
  web_ui()->RegisterMessageCallback(
      "getMetrics",
      base::Bind(&PerformanceMonitorHandler::HandleGetMetrics,
                 AsWeakPtr()));
}

void PerformanceMonitorHandler::ReturnResults(const std::string& function,
                                 const Value* results) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  web_ui()->CallJavascriptFunction(function, *results);
}

void PerformanceMonitorHandler::HandleGetActiveIntervals(
    const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(2u, args->GetSize());
  double double_time = 0.0;
  CHECK(args->GetDouble(0, &double_time));
  base::Time start = base::Time::FromJsTime(double_time);
  CHECK(args->GetDouble(1, &double_time));
  base::Time end = base::Time::FromJsTime(double_time);

  ListValue* results = new ListValue();
  util::PostTaskToDatabaseThreadAndReply(
      FROM_HERE,
      base::Bind(&DoGetActiveIntervals, results, start, end),
      base::Bind(&PerformanceMonitorHandler::ReturnResults, AsWeakPtr(),
                 "PerformanceMonitor.getActiveIntervalsCallback",
                 base::Owned(results)));
}

void PerformanceMonitorHandler::HandleGetFlagEnabled(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(0u, args->GetSize());
  scoped_ptr<Value> value(Value::CreateBooleanValue(
      CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kPerformanceMonitorGathering)));
  ReturnResults("PerformanceMonitor.getFlagEnabledCallback", value.get());
}

void PerformanceMonitorHandler::HandleGetAggregationTypes(
    const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(0u, args->GetSize());
  ListValue results;
  for (int i = 0; i < AGGREGATION_METHOD_NUMBER_OF_METHODS; ++i) {
    results.Append(
        GetAggregationMethod(static_cast<AggregationMethod>(i)).release());
  }

  ReturnResults(
      "PerformanceMonitor.getAggregationTypesCallback", &results);
}

void PerformanceMonitorHandler::HandleGetEventTypes(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(0u, args->GetSize());
  ListValue results;
  for (int i = 0; i < EVENT_CATEGORY_NUMBER_OF_CATEGORIES; ++i)
    results.Append(GetEventCategory(static_cast<EventCategory>(i)).release());

  ReturnResults("PerformanceMonitor.getEventTypesCallback", &results);
}

void PerformanceMonitorHandler::HandleGetEvents(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(3u, args->GetSize());

  const ListValue* event_type_list;
  CHECK(args->GetList(0, &event_type_list));
  std::set<EventType> event_types;
  for (ListValue::const_iterator iter = event_type_list->begin();
       iter != event_type_list->end(); ++iter) {
    double event_type_double = 0.0;
    CHECK((*iter)->GetAsDouble(&event_type_double));
    CHECK(event_type_double < EVENT_NUMBER_OF_EVENTS &&
        event_type_double > EVENT_UNDEFINED);
    event_types.insert(
        static_cast<EventType>(static_cast<int>(event_type_double)));
  }

  double double_time = 0.0;
  CHECK(args->GetDouble(1, &double_time));
  base::Time start = base::Time::FromJsTime(double_time);
  CHECK(args->GetDouble(2, &double_time));
  base::Time end = base::Time::FromJsTime(double_time);

  ListValue* results = new ListValue();
  util::PostTaskToDatabaseThreadAndReply(
      FROM_HERE,
      base::Bind(&DoGetEvents, results, event_types, start, end),
      base::Bind(&PerformanceMonitorHandler::ReturnResults, AsWeakPtr(),
                 "PerformanceMonitor.getEventsCallback",
                 base::Owned(results)));
}

void PerformanceMonitorHandler::HandleGetMetricTypes(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(0u, args->GetSize());
  ListValue results;
  for (int i = 0; i < METRIC_CATEGORY_NUMBER_OF_CATEGORIES; ++i)
    results.Append(GetMetricCategory(static_cast<MetricCategory>(i)).release());

  ReturnResults("PerformanceMonitor.getMetricTypesCallback", &results);
}

void PerformanceMonitorHandler::HandleGetMetrics(const ListValue* args) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  CHECK_EQ(5u, args->GetSize());

  const ListValue* metric_type_list;
  CHECK(args->GetList(0, &metric_type_list));
  std::set<MetricType> metric_types;
  for (ListValue::const_iterator iter = metric_type_list->begin();
       iter != metric_type_list->end(); ++iter) {
    double metric_type_double = 0.0;
    CHECK((*iter)->GetAsDouble(&metric_type_double));
    CHECK(metric_type_double < METRIC_NUMBER_OF_METRICS &&
          metric_type_double > METRIC_UNDEFINED);
    metric_types.insert(
        static_cast<MetricType>(static_cast<int>(metric_type_double)));
  }

  double time_double = 0.0;
  CHECK(args->GetDouble(1, &time_double));
  base::Time start = base::Time::FromJsTime(time_double);
  CHECK(args->GetDouble(2, &time_double));
  base::Time end = base::Time::FromJsTime(time_double);

  double resolution_in_milliseconds = 0.0;
  CHECK(args->GetDouble(3, &resolution_in_milliseconds));
  base::TimeDelta resolution =
      base::TimeDelta::FromMilliseconds(resolution_in_milliseconds);

  double aggregation_double = 0.0;
  CHECK(args->GetDouble(4, &aggregation_double));
  CHECK(aggregation_double < AGGREGATION_METHOD_NUMBER_OF_METHODS &&
        aggregation_double >= 0);
  AggregationMethod aggregation_method =
      static_cast<AggregationMethod>(static_cast<int>(aggregation_double));

  ListValue* results = new ListValue();
  util::PostTaskToDatabaseThreadAndReply(
      FROM_HERE,
      base::Bind(&DoGetMetrics, results, metric_types,
                 start, end, resolution, aggregation_method),
      base::Bind(&PerformanceMonitorHandler::ReturnResults, AsWeakPtr(),
                 "PerformanceMonitor.getMetricsCallback",
                 base::Owned(results)));
}

}  // namespace performance_monitor
