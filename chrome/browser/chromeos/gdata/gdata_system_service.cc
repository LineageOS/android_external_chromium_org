// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/gdata/gdata_system_service.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/download/download_service.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/chromeos/gdata/gdata_documents_service.h"
#include "chrome/browser/chromeos/gdata/gdata_download_observer.h"
#include "chrome/browser/chromeos/gdata/gdata_file_system.h"
#include "chrome/browser/chromeos/gdata/gdata_sync_client.h"
#include "chrome/browser/chromeos/gdata/gdata_uploader.h"
#include "chrome/browser/profiles/profile_dependency_manager.h"

namespace gdata {

//===================== GDataSystemService ====================================
GDataSystemService::GDataSystemService(Profile* profile)
    : profile_(profile),
      file_system_(new GDataFileSystem(profile, new DocumentsService)),
      uploader_(new GDataUploader(file_system_.get())),
      download_observer_(new GDataDownloadObserver),
      sync_client_(new GDataSyncClient(file_system_.get())) {
}

GDataSystemService::~GDataSystemService() {
}

void GDataSystemService::Initialize() {
  file_system_->Initialize();

  content::DownloadManager* download_manager =
    g_browser_process->download_status_updater() ?
        DownloadServiceFactory::GetForProfile(profile_)->GetDownloadManager() :
        NULL;
  download_observer_->Initialize(
      file_system_->GetGDataTempDownloadFolderPath(),
      uploader_.get(), download_manager);

  sync_client_->Initialize();
}

void GDataSystemService::Shutdown() {
  file_system_->Shutdown();
}

//===================== GDataSystemServiceFactory =============================

// static
GDataSystemService* GDataSystemServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<GDataSystemService*>(
      GetInstance()->GetServiceForProfile(profile, true));
}

// static
GDataSystemService* GDataSystemServiceFactory::FindForProfile(
    Profile* profile) {
  return static_cast<GDataSystemService*>(
      GetInstance()->GetServiceForProfile(profile, false));
}

// static
GDataSystemServiceFactory* GDataSystemServiceFactory::GetInstance() {
  return Singleton<GDataSystemServiceFactory>::get();
}

GDataSystemServiceFactory::GDataSystemServiceFactory()
    : ProfileKeyedServiceFactory("GDataSystemService",
                                 ProfileDependencyManager::GetInstance()) {
  DependsOn(DownloadServiceFactory::GetInstance());
}

GDataSystemServiceFactory::~GDataSystemServiceFactory() {
}

ProfileKeyedService* GDataSystemServiceFactory::BuildServiceInstanceFor(
    Profile* profile) const {
  GDataSystemService* service = new GDataSystemService(profile);
  service->Initialize();
  return service;
}

}  // namespace gdata
