// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/cloud_print/cloud_print_helpers.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/rand_util.h"
#include "base/stringprintf.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/cloud_print/cloud_print_constants.h"
#include "googleurl/src/gurl.h"

namespace cloud_print {

namespace {

// Returns printer tags generated from |printer_tags| and the default tags
// required by cloud print server.
PrinterTags PreparePrinterTags(const PrinterTags& printer_tags) {
  PrinterTags printer_tags_out = printer_tags;
  chrome::VersionInfo version_info;
  DCHECK(version_info.is_valid());
  printer_tags_out[kChromeVersionTagName] =
      version_info.CreateVersionString();
  printer_tags_out[kSystemNameTagName] =
      base::SysInfo::OperatingSystemName();
  printer_tags_out[kSystemVersionTagName] =
      base::SysInfo::OperatingSystemVersion();
  return printer_tags_out;
}

// Returns the hash of |printer_tags|.
std::string HashPrinterTags(const PrinterTags& printer_tags) {
  std::string values_list;
  PrinterTags::const_iterator it;
  for (it = printer_tags.begin(); it != printer_tags.end(); ++it) {
    values_list.append(it->first);
    values_list.append(it->second);
  }
  return base::MD5String(values_list);
}

}  // namespace

std::string AppendPathToUrl(const GURL& url, const std::string& path) {
  DCHECK_NE(path[0], '/');
  std::string ret = url.path();
  if (url.has_path() && (ret[ret.length() - 1] != '/'))
    ret += '/';
  ret += path;
  return ret;
}

GURL GetUrlForSearch(const GURL& cloud_print_server_url) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "search"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForSubmit(const GURL& cloud_print_server_url) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "submit"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForPrinterList(const GURL& cloud_print_server_url,
                          const std::string& proxy_id) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "list"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf("proxy=%s", proxy_id.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForPrinterRegistration(const GURL& cloud_print_server_url) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "register"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForPrinterUpdate(const GURL& cloud_print_server_url,
                            const std::string& printer_id) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "update"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf("printerid=%s", printer_id.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForPrinterDelete(const GURL& cloud_print_server_url,
                            const std::string& printer_id,
                            const std::string& reason) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "delete"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf(
      "printerid=%s&reason=%s", printer_id.c_str(), reason.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForJobFetch(const GURL& cloud_print_server_url,
                       const std::string& printer_id,
                       const std::string& reason) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "fetch"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf(
      "printerid=%s&deb=%s", printer_id.c_str(), reason.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}


GURL GetUrlForJobDelete(const GURL& cloud_print_server_url,
                        const std::string& job_id) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "deletejob"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf("jobid=%s", job_id.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForJobStatusUpdate(const GURL& cloud_print_server_url,
                              const std::string& job_id,
                              const std::string& status_string) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "control"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf(
      "jobid=%s&status=%s", job_id.c_str(), status_string.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForUserMessage(const GURL& cloud_print_server_url,
                          const std::string& message_id) {
  std::string path(AppendPathToUrl(cloud_print_server_url, "message"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf("code=%s", message_id.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

GURL GetUrlForGetAuthCode(const GURL& cloud_print_server_url,
                          const std::string& oauth_client_id,
                          const std::string& proxy_id) {
  // We use the internal API "createrobot" instead of "getauthcode". This API
  // will add the robot as owner to all the existing printers for this user.
  std::string path(AppendPathToUrl(cloud_print_server_url, "createrobot"));
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  std::string query = StringPrintf("oauth_client_id=%s&proxy=%s",
                                    oauth_client_id.c_str(),
                                    proxy_id.c_str());
  replacements.SetQueryStr(query);
  return cloud_print_server_url.ReplaceComponents(replacements);
}

bool ParseResponseJSON(const std::string& response_data,
                       bool* succeeded,
                       DictionaryValue** response_dict) {
  scoped_ptr<Value> message_value(base::JSONReader::Read(response_data));
  if (!message_value.get())
    return false;

  if (!message_value->IsType(Value::TYPE_DICTIONARY))
    return false;

  scoped_ptr<DictionaryValue> response_dict_local(
      static_cast<DictionaryValue*>(message_value.release()));
  if (succeeded &&
      !response_dict_local->GetBoolean(kSuccessValue, succeeded))
    *succeeded = false;
  if (response_dict)
    *response_dict = response_dict_local.release();
  return true;
}

void AddMultipartValueForUpload(const std::string& value_name,
                                const std::string& value,
                                const std::string& mime_boundary,
                                const std::string& content_type,
                                std::string* post_data) {
  DCHECK(post_data);
  // First line is the boundary
  post_data->append("--" + mime_boundary + "\r\n");
  // Next line is the Content-disposition
  post_data->append(StringPrintf("Content-Disposition: form-data; "
                   "name=\"%s\"\r\n", value_name.c_str()));
  if (!content_type.empty()) {
    // If Content-type is specified, the next line is that
    post_data->append(StringPrintf("Content-Type: %s\r\n",
                      content_type.c_str()));
  }
  // Leave an empty line and append the value.
  post_data->append(StringPrintf("\r\n%s\r\n", value.c_str()));
}

std::string GetMultipartMimeType(const std::string& mime_boundary) {
  return std::string("multipart/form-data; boundary=") + mime_boundary;
}

// Create a MIME boundary marker (27 '-' characters followed by 16 hex digits).
void CreateMimeBoundaryForUpload(std::string* out) {
  int r1 = base::RandInt(0, kint32max);
  int r2 = base::RandInt(0, kint32max);
  base::SStringPrintf(out, "---------------------------%08X%08X", r1, r2);
}

std::string GetHashOfPrinterTags(const PrinterTags& printer_tags) {
  return HashPrinterTags(PreparePrinterTags(printer_tags));
}

std::string GetPostDataForPrinterTags(
    const PrinterTags& printer_tags,
    const std::string& mime_boundary,
    const std::string& proxy_tag_prefix,
    const std::string& tags_hash_tag_name) {
  PrinterTags printer_tags_prepared = PreparePrinterTags(printer_tags);
  std::string post_data;
  for (PrinterTags::const_iterator it = printer_tags_prepared.begin();
       it != printer_tags_prepared.end(); ++it) {
    // TODO(gene) Escape '=' char from name. Warning for now.
    if (it->first.find('=') != std::string::npos) {
      LOG(WARNING) <<
          "CP_PROXY: Printer option name contains '=' character";
      NOTREACHED();
    }
    // All our tags have a special prefix to identify them as such.
    std::string msg = StringPrintf("%s%s=%s",
        proxy_tag_prefix.c_str(), it->first.c_str(), it->second.c_str());
    AddMultipartValueForUpload(kPrinterTagValue, msg, mime_boundary,
        std::string(), &post_data);
  }
  std::string tags_hash_msg = StringPrintf("%s=%s",
      tags_hash_tag_name.c_str(),
      HashPrinterTags(printer_tags_prepared).c_str());
  AddMultipartValueForUpload(kPrinterTagValue, tags_hash_msg, mime_boundary,
      std::string(), &post_data);
  return post_data;
}

std::string GetCloudPrintAuthHeader(const std::string& auth_token) {
  return StringPrintf("Authorization: OAuth %s", auth_token.c_str());
}

}  // namespace cloud_print
