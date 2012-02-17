// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DBUS_PROPERTY_H_
#define DBUS_PROPERTY_H_
#pragma once

#include <map>
#include <string>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/callback.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

// D-Bus objects frequently provide sets of properties accessed via a
// standard interface of method calls and signals to obtain the current value,
// set a new value and be notified of changes to the value. Unfortunately this
// interface makes heavy use of variants and dictionaries of variants. The
// classes defined here make dealing with properties in a type-safe manner
// possible.
//
// Client implementation classes should define a Properties structure, deriving
// from the PropertySet class defined here. This structure should contain a
// member for each property defined as an instance of the Property<> class,
// specifying the type to the template. Finally the structure should chain up
// to the PropertySet constructor, and then call RegisterProperty() for each
// property defined to associate them with their string name.
//
// Example:
//   class ExampleClient {
//    public:
//     struct Properties : public dbus::PropertySet {
//       dbus::Property<std::string> name;
//       dbus::Property<uint16> version;
//       dbus::Property<dbus::ObjectPath> parent;
//       dbus::Property<std::vector<std::string> > children;
//
//       Properties(dbus::ObjectProxy* object_proxy,
//                  PropertyChangedCallback callback)
//           : dbus::PropertySet(object_proxy, "com.example.DBus", callback) {
//         RegisterProperty("Name", &name);
//         RegisterProperty("Version", &version);
//         RegisterProperty("Parent", &parent);
//         RegisterProperty("Children", &children);
//       }
//       virtual ~Properties() {}
//     };
//
// The Properties structure requires a pointer to the object proxy of the
// actual object to track, and after construction should have signals
// connected to that object and initial values set by calling ConnectSignals()
// and GetAll(). The structure should not outlive the object proxy, so it
// is recommended that the lifecycle of both be managed together.
//
// Example (continued):
//
//     typedef std::map<std::pair<dbus::ObjectProxy*, Properties*> > Object;
//     typedef std::map<dbus::ObjectPath, Object> ObjectMap;
//     ObjectMap object_map_;
//
//     dbus::ObjectProxy* GetObjectProxy(const dbus::ObjectPath& object_path) {
//       return GetObject(object_path).first;
//     }
//
//     Properties* GetProperties(const dbus::ObjectPath& object_path) {
//       return GetObject(object_path).second;
//     }
//
//     Object GetObject(const dbus::ObjectPath& object_path) {
//       ObjectMap::iterator it = object_map_.find(object_path);
//       if (it != object_map_.end())
//         return it->second;
//
//       dbus::ObjectProxy* object_proxy = bus->GetObjectProxy(...);
//       // connect signals, etc.
//
//       Properties* properties = new Properties(
//           object_proxy,
//           base::Bind(&PropertyChanged,
//                      weak_ptr_factory_.GetWeakPtr(),
//                      object_path));
//       properties->ConnectSignals();
//       properties->GetAll();
//
//       Object object = std::make_pair(object_proxy, properties);
//       object_map_[object_path] = object;
//       return object;
//     }
//  };
//
// This now allows code using the client implementation to access properties
// in a type-safe manner, and assuming the PropertyChanged callback is
// propogated up to observers, be notified of changes. A typical access of
// the current value of the name property would be:
//
//   ExampleClient::Properties* p = example_client->GetProperties(object_path);
//   std::string name = p->name.value();
//
// Normally these values are updated from signals emitted by the remote object,
// in case an explicit round-trip is needed to obtain the current value, the
// Get() method can be used and indicates whether or not the value update was
// successful. The updated value can be obtained in the callback using the
// value() method.
//
//   p->children.Get(base::Bind(&OnGetChildren));
//
// A new value can be set using the Set() method, the callback indicates
// success only; it is up to the remote object when (and indeed if) it updates
// the property value, and whether it emits a signal or a Get() call is
// required to obtain it.
//
//   p->version.Set(20, base::Bind(&OnSetVersion))

namespace dbus {

// D-Bus Properties interface constants, declared here rather than
// in property.cc because template methods use them.
const char kPropertiesInterface[] = "org.freedesktop.DBus.Properties";
const char kPropertiesGetAll[] = "GetAll";
const char kPropertiesGet[] = "Get";
const char kPropertiesSet[] = "Set";
const char kPropertiesChanged[] = "PropertiesChanged";

class PropertySet;

// PropertyBase is an abstract base-class consisting of the parts of
// the Property<> template that are not type-specific, such as the
// associated PropertySet, property name, and the type-unsafe parts
// used by PropertySet.
class PropertyBase {
 public:
  PropertyBase() : property_set_(NULL) {}

  // Initializes the |property_set| and property |name| so that method
  // calls may be made from this class. This method is called by
  // PropertySet::RegisterProperty(), there should be no need to call it
  // directly.
  void Init(PropertySet* property_set, const std::string& name);

  // Retrieves the name of this property, this may be useful in observers
  // to avoid specifying the name in more than once place, e.g.
  //
  //   void Client::PropertyChanged(const dbus::ObjectPath& object_path,
  //                                const std::string &property_name) {
  //     Properties& properties = GetProperties(object_path);
  //     if (property_name == properties.version.name()) {
  //       // Handle version property changing
  //     }
  //   }
  const std::string& name() { return name_; }

  // Method used by PropertySet to retrieve the value from a MessageReader,
  // no knowledge of the contained type is required, this method returns
  // true if its expected type was found, false if not.
  virtual bool PopValueFromReader(MessageReader*) = 0;

 protected:
  // Retrieves the associated property set.
  PropertySet* property_set() { return property_set_; }

 private:
  // Pointer to the associated property set.
  PropertySet* property_set_;

  // Name of the property.
  std::string name_;

  DISALLOW_COPY_AND_ASSIGN(PropertyBase);
};

// PropertySet groups a collection of properties for a remote object
// together into a single structure, fixing their types and name such
// that calls made through it are type-safe.
//
// Clients always sub-class this to add the properties, and should always
// provide a constructor that chains up to this and then calls
// RegisterProperty() for each property defined.
//
// After creation, client code should call ConnectSignals() and most likely
// GetAll() to seed initial values and update as changes occur.
class PropertySet {
 public:
  // Callback for changes to cached values of properties, either notified
  // via signal, or as a result of calls to Get() and GetAll(). The |name|
  // argument specifies the name of the property changed.
  typedef base::Callback<void(const std::string& name)> PropertyChangedCallback;

  // Construct a property set, |object_proxy| specifies the proxy for the
  // remote object that these properties are for, care should be taken to
  // ensure that this object does not outlive the lifetime of the proxy;
  // |interface| specifies the D-Bus interface of these properties, and
  // |property_changed_callback| specifies the callback for when properties
  // are changed, this may be a NULL callback.
  PropertySet(ObjectProxy* object_proxy, const std::string& interface,
              PropertyChangedCallback property_changed_callback);

  // Destructor; we don't hold on to any references or memory that needs
  // explicit clean-up, but clang thinks we might.
  virtual ~PropertySet();

  // Registers a property, generally called from the subclass constructor;
  // pass the |name| of the property as used in method calls and signals,
  // and the pointer to the |property| member of the structure. This will
  // call the PropertyBase::Init method.
  void RegisterProperty(const std::string& name, PropertyBase* property);

  // Call after construction to connect property change notification
  // signals. Sub-classes may override to use different D-Bus signals.
  void ConnectSignals();

  // Methods connected by ConnectSignals() and called by dbus:: when
  // a property is changed. Sub-classes may override if the property
  // changed signal provides different arguments.
  virtual void ChangedReceived(Signal*);
  virtual void ChangedConnected(const std::string& interface_name,
                                const std::string& signal_name,
                                bool success);

  // Queries the remote object for values of all properties and updates
  // initial values. Sub-classes may override to use a different D-Bus
  // method, or if the remote object does not support retrieving all
  // properties, either ignore or obtain each property value individually.
  void GetAll();
  virtual void OnGetAll(Response* response);

  // Update properties by reading an array of dictionary entries, each
  // containing a string with the name and a variant with the value, from
  // |message_reader|. Returns false if message in incorrect format.
  bool UpdatePropertiesFromReader(MessageReader* reader);

  // Updates a single property by reading a string with the name and a
  // variant with the value from |message_reader|. Returns false if message
  // in incorrect format, or property type doesn't match.
  bool UpdatePropertyFromReader(MessageReader* reader);

  // Calls the property changed callback passed to the constructor, used
  // by sub-classes that do not call UpdatePropertiesFromReader() or
  // UpdatePropertyFromReader(). Takes the |name| of the changed property.
  void NotifyPropertyChanged(const std::string& name);

  // Retrieves the object proxy this property set was initialized with,
  // provided for sub-classes overriding methods that make D-Bus calls
  // and for Property<>.
  ObjectProxy* object_proxy() { return object_proxy_; }

  // Retrieves the interface of this property set.
  const std::string& interface() { return interface_; }

 protected:
  // Get a weak pointer to this property set, provided so that sub-classes
  // overriding methods that make D-Bus calls may use the existing (or
  // override) callbacks without providing their own weak pointer factory.
  base::WeakPtr<PropertySet> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  // Pointer to object proxy for making method calls.
  ObjectProxy* object_proxy_;

  // Interface of property, e.g. "org.chromium.ExampleService", this is
  // distinct from the interface of the method call itself which is the
  // general D-Bus Properties interface "org.freedesktop.DBus.Properties".
  std::string interface_;

  // Callback for property changes.
  PropertyChangedCallback property_changed_callback_;

  // Map of properties (as PropertyBase*) defined in the structure to
  // names as used in D-Bus method calls and signals. The base pointer
  // restricts property access via this map to type-unsafe and non-specific
  // actions only.
  typedef std::map<const std::string, PropertyBase*> PropertiesMap;
  PropertiesMap properties_map_;

  // Weak pointer factory as D-Bus callbacks may last longer than these
  // objects.
  base::WeakPtrFactory<PropertySet> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PropertySet);
};

// Property template, this defines the type-specific and type-safe methods
// of properties that can be accessed as members of a PropertySet structure.
//
// Properties provide a cached value that has an initial sensible default
// until the reply to PropertySet::GetAll() is retrieved and is updated by
// all calls to that method, Property<>::Get() and property changed signals
// handled by PropertySet. It can be obtained by calling value() on the
// property.
//
// It is recommended that this cached value be used where necessary, with
// code using PropertySet::PropertyChangedCallback to be notified of changes,
// rather than incurring a round-trip to the remote object for each property
// access.
//
// Where a round-trip is necessary, the Get() method is provided. And to
// update the remote object value, the Set() method is also provided.
//
// Handling of particular D-Bus types is performed via specialization,
// typically the PopValueFromReader() and AppendToWriter() methods will need
// to be provided, and in rare cases a constructor to provide a default value.
// Specializations for basic D-Bus types, strings, object paths and arrays
// are provided for you.
template <class T>
class Property : public PropertyBase {
 public:
  // Callback for Get() method, |success| indicates whether or not the
  // value could be retrived, if true the new value can be obtained by
  // calling value() on the property.
  typedef base::Callback<void(bool success)> GetCallback;

  // Callback for Set() method, |success| indicates whether or not the
  // new property value was accepted by the remote object.
  typedef base::Callback<void(bool success)> SetCallback;

  Property() : weak_ptr_factory_(this) {}

  // Retrieves the cached value.
  const T& value() const { return value_; }

  // Requests an updated value from the remote object incurring a
  // round-trip. |callback| will be called when the new value is available.
  // This may not be implemented by some interfaces, and may be overriden by
  // sub-classes if interfaces use different method calls.
  void Get(GetCallback callback) {
    MethodCall method_call(kPropertiesInterface, kPropertiesGet);
    MessageWriter writer(&method_call);
    writer.AppendString(property_set()->interface());
    writer.AppendString(name());

    ObjectProxy* object_proxy = property_set()->object_proxy();
    DCHECK(object_proxy);
    object_proxy->CallMethod(&method_call,
                             ObjectProxy::TIMEOUT_USE_DEFAULT,
                             base::Bind(&Property<T>::OnGet,
                                        GetWeakPtr(),
                                        callback));
  }

  // Callback for Get(), may be overriden by sub-classes if interfaces
  // use different response arguments.
  virtual void OnGet(SetCallback callback, Response* response) {
    if (!response) {
      LOG(WARNING) << name() << ": Get: failed.";
      return;
    }

    MessageReader reader(response);
    if (PopValueFromReader(&reader))
      property_set()->NotifyPropertyChanged(name());

    if (!callback.is_null())
      callback.Run(response);
  }

  // Requests that the remote object change the property value to |value|,
  // |callback| will be called to indicate the success or failure of the
  // request, however the new value may not be available depending on the
  // remote object. This method may be overriden by sub-classes if
  // interfaces use different method calls.
  void Set(const T& value, SetCallback callback) {
    MethodCall method_call(kPropertiesInterface, kPropertiesSet);
    MessageWriter writer(&method_call);
    writer.AppendString(property_set()->interface());
    writer.AppendString(name());
    AppendToWriter(&writer, value);

    ObjectProxy* object_proxy = property_set()->object_proxy();
    DCHECK(object_proxy);
    object_proxy->CallMethod(&method_call,
                             ObjectProxy::TIMEOUT_USE_DEFAULT,
                             base::Bind(&Property<T>::OnSet,
                                        GetWeakPtr(),
                                        callback));
  }

  // Callback for Set(), may be overriden by sub-classes if interfaces
  // use different response arguments.
  virtual void OnSet(SetCallback callback, Response* response) {
    LOG_IF(WARNING, !response) << name() << ": Set: failed.";
    if (!callback.is_null())
      callback.Run(response);
  }

  // Updates the cached property value by popping from |reader| which
  // should be positioned at the property value, generally of variant
  // type. Implementation provided by specialization.
  virtual bool PopValueFromReader(MessageReader* reader);

  // Appends the passed |value| to |writer|, generally as a variant type.
  // Implementation provided by specialization.
  virtual void AppendToWriter(MessageWriter* writer, const T& value);

 protected:
  // Get a weak pointer to this propertyt, provided so that sub-classes
  // overriding methods that make D-Bus calls may use the existing (or
  // override) callbacks without providing their own weak pointer factory.
  base::WeakPtr<Property<T> > GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  // Current cached value of the property.
  T value_;

  // Weak pointer factory as D-Bus callbacks may last longer than these
  // objects.
  base::WeakPtrFactory<Property<T> > weak_ptr_factory_;
};

}  // namespace dbus

#endif  // DBUS_PROPERTY_H_
