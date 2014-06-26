// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/image_writer_private/removable_storage_provider.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/storage/IOBlockStorageDevice.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOStorageProtocolCharacteristics.h>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_ioobject.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/thread_restrictions.h"

namespace extensions {

// static
bool RemovableStorageProvider::PopulateDeviceList(
    scoped_refptr<StorageDeviceList> device_list) {
  base::ThreadRestrictions::AssertIOAllowed();
  // Match only removable, ejectable, non-internal, whole disks.
  CFMutableDictionaryRef matching = IOServiceMatching(kIOMediaClass);
  CFDictionaryAddValue(matching, CFSTR(kIOMediaWholeKey), kCFBooleanTrue);
  CFDictionaryAddValue(matching, CFSTR(kIOMediaEjectableKey), kCFBooleanTrue);
  CFDictionaryAddValue(matching, CFSTR(kIOMediaRemovableKey), kCFBooleanTrue);
  CFDictionaryAddValue(matching, CFSTR(kIOMediaWritableKey), kCFBooleanTrue);

  io_service_t disk_iterator;
  if (IOServiceGetMatchingServices(
          kIOMasterPortDefault, matching, &disk_iterator) != KERN_SUCCESS) {
    LOG(ERROR) << "Unable to get disk services.";
    return false;
  }
  base::mac::ScopedIOObject<io_service_t> iterator_ref(disk_iterator);

  io_object_t disk_obj;
  while ((disk_obj = IOIteratorNext(disk_iterator))) {
    base::mac::ScopedIOObject<io_object_t> disk_obj_ref(disk_obj);

    CFMutableDictionaryRef dict;
    if (IORegistryEntryCreateCFProperties(
            disk_obj, &dict, kCFAllocatorDefault, 0) != KERN_SUCCESS) {
      LOG(ERROR) << "Unable to get properties of disk object.";
      continue;
    }
    base::ScopedCFTypeRef<CFMutableDictionaryRef> dict_ref(dict);

    CFStringRef cf_bsd_name = base::mac::GetValueFromDictionary<CFStringRef>(
        dict, CFSTR(kIOBSDNameKey));
    std::string bsd_name = base::SysCFStringRefToUTF8(cf_bsd_name);

    CFNumberRef size_number = base::mac::GetValueFromDictionary<CFNumberRef>(
        dict, CFSTR(kIOMediaSizeKey));
    uint64 size_in_bytes = 0;
    if (size_number)
      CFNumberGetValue(size_number, kCFNumberLongLongType, &size_in_bytes);

    base::ScopedCFTypeRef<CFDictionaryRef> characteristics(
        static_cast<CFDictionaryRef>(IORegistryEntrySearchCFProperty(
            disk_obj,
            kIOServicePlane,
            CFSTR(kIOPropertyDeviceCharacteristicsKey),
            kCFAllocatorDefault,
            kIORegistryIterateParents | kIORegistryIterateRecursively)));

    if (characteristics == NULL) {
      LOG(ERROR) << "Unable to find device characteristics for " << cf_bsd_name;
      continue;
    }

    CFStringRef cf_vendor = base::mac::GetValueFromDictionary<CFStringRef>(
        characteristics, CFSTR(kIOPropertyVendorNameKey));
    std::string vendor = base::SysCFStringRefToUTF8(cf_vendor);

    CFStringRef cf_model = base::mac::GetValueFromDictionary<CFStringRef>(
        characteristics, CFSTR(kIOPropertyProductNameKey));
    std::string model = base::SysCFStringRefToUTF8(cf_model);

    linked_ptr<api::image_writer_private::RemovableStorageDevice> device(
        new api::image_writer_private::RemovableStorageDevice());
    device->storage_unit_id = bsd_name;
    device->capacity = size_in_bytes;
    device->vendor = vendor;
    device->model = model;
    device_list->data.push_back(device);
  }

  return true;
}

} // namespace extensions
