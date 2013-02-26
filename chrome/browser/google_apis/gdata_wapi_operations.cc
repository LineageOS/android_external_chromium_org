// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/google_apis/gdata_wapi_operations.h"

#include "base/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"
#include "chrome/browser/google_apis/gdata_wapi_url_generator.h"
#include "chrome/browser/google_apis/time_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/base/url_util.h"
#include "third_party/libxml/chromium/libxml_utils.h"

using content::BrowserThread;
using net::URLFetcher;

namespace google_apis {

namespace {

// etag matching header.
const char kIfMatchAllHeader[] = "If-Match: *";
const char kIfMatchHeaderPrefix[] = "If-Match: ";

const char kUploadContentRange[] = "Content-Range: bytes ";

const char kFeedField[] = "feed";

// Returns If-Match header string for |etag|.
// If |etag| is empty, the returned header should match any etag.
std::string GenerateIfMatchHeader(const std::string& etag) {
  return etag.empty() ? kIfMatchAllHeader : (kIfMatchHeaderPrefix + etag);
}

// Parses the |value| to ResourceEntry with error handling.
// This is designed to be used for ResumeUploadOperation and
// GetUploadStatusOperation.
scoped_ptr<ResourceEntry> ParseResourceEntry(scoped_ptr<base::Value> value) {
  scoped_ptr<ResourceEntry> entry;
  if (value.get()) {
    entry = ResourceEntry::ExtractAndParse(*value);

    // Note: |value| may be NULL, in particular if the callback is for a
    // failure.
    if (!entry.get())
      LOG(WARNING) << "Invalid entry received on upload.";
  }

  return entry.Pass();
}

}  // namespace

//============================ Structs ===========================

UploadRangeResponse::UploadRangeResponse()
    : code(HTTP_SUCCESS),
      start_position_received(0),
      end_position_received(0) {
}

UploadRangeResponse::UploadRangeResponse(GDataErrorCode code,
                                         int64 start_position_received,
                                         int64 end_position_received)
    : code(code),
      start_position_received(start_position_received),
      end_position_received(end_position_received) {
}

UploadRangeResponse::~UploadRangeResponse() {
}

ResumeUploadParams::ResumeUploadParams(
    UploadMode upload_mode,
    int64 start_position,
    int64 end_position,
    int64 content_length,
    const std::string& content_type,
    scoped_refptr<net::IOBuffer> buf,
    const GURL& upload_location,
    const base::FilePath& drive_file_path) : upload_mode(upload_mode),
                                    start_position(start_position),
                                    end_position(end_position),
                                    content_length(content_length),
                                    content_type(content_type),
                                    buf(buf),
                                    upload_location(upload_location),
                                    drive_file_path(drive_file_path) {
}

ResumeUploadParams::~ResumeUploadParams() {
}

//============================ GetResourceListOperation ========================

GetResourceListOperation::GetResourceListOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const GURL& override_url,
    int start_changestamp,
    const std::string& search_string,
    bool shared_with_me,
    const std::string& directory_resource_id,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      override_url_(override_url),
      start_changestamp_(start_changestamp),
      search_string_(search_string),
      shared_with_me_(shared_with_me),
      directory_resource_id_(directory_resource_id) {
  DCHECK(!callback.is_null());
}

GetResourceListOperation::~GetResourceListOperation() {}

GURL GetResourceListOperation::GetURL() const {
  return url_generator_.GenerateResourceListUrl(override_url_,
                                                start_changestamp_,
                                                search_string_,
                                                shared_with_me_,
                                                directory_resource_id_);
}

//============================ GetResourceEntryOperation =======================

GetResourceEntryOperation::GetResourceEntryOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const std::string& resource_id,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(resource_id) {
  DCHECK(!callback.is_null());
}

GetResourceEntryOperation::~GetResourceEntryOperation() {}

GURL GetResourceEntryOperation::GetURL() const {
  return url_generator_.GenerateEditUrl(resource_id_);
}

//========================= GetAccountMetadataOperation ========================

GetAccountMetadataOperation::GetAccountMetadataOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const GetDataCallback& callback)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

GetAccountMetadataOperation::~GetAccountMetadataOperation() {}

GURL GetAccountMetadataOperation::GetURL() const {
  return url_generator_.GenerateAccountMetadataUrl();
}

//=========================== DeleteResourceOperation ==========================

DeleteResourceOperation::DeleteResourceOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const EntryActionCallback& callback,
    const std::string& resource_id,
    const std::string& etag)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(resource_id),
      etag_(etag) {
  DCHECK(!callback.is_null());
}

DeleteResourceOperation::~DeleteResourceOperation() {}

GURL DeleteResourceOperation::GetURL() const {
  return url_generator_.GenerateEditUrl(resource_id_);
}

URLFetcher::RequestType DeleteResourceOperation::GetRequestType() const {
  return URLFetcher::DELETE_REQUEST;
}

std::vector<std::string>
DeleteResourceOperation::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(GenerateIfMatchHeader(etag_));
  return headers;
}

//========================== CreateDirectoryOperation ==========================

CreateDirectoryOperation::CreateDirectoryOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const GetDataCallback& callback,
    const std::string& parent_resource_id,
    const std::string& directory_name)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      directory_name_(directory_name) {
  DCHECK(!callback.is_null());
}

CreateDirectoryOperation::~CreateDirectoryOperation() {}

GURL CreateDirectoryOperation::GetURL() const {
  return url_generator_.GenerateContentUrl(parent_resource_id_);
}

URLFetcher::RequestType
CreateDirectoryOperation::GetRequestType() const {
  return URLFetcher::POST;
}

bool CreateDirectoryOperation::GetContentData(std::string* upload_content_type,
                                              std::string* upload_content) {
  upload_content_type->assign("application/atom+xml");
  XmlWriter xml_writer;
  xml_writer.StartWriting();
  xml_writer.StartElement("entry");
  xml_writer.AddAttribute("xmlns", "http://www.w3.org/2005/Atom");

  xml_writer.StartElement("category");
  xml_writer.AddAttribute("scheme",
                          "http://schemas.google.com/g/2005#kind");
  xml_writer.AddAttribute("term",
                          "http://schemas.google.com/docs/2007#folder");
  xml_writer.EndElement();  // Ends "category" element.

  xml_writer.WriteElement("title", directory_name_);

  xml_writer.EndElement();  // Ends "entry" element.
  xml_writer.StopWriting();
  upload_content->assign(xml_writer.GetWrittenString());
  DVLOG(1) << "CreateDirectory data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//============================ CopyHostedDocumentOperation =====================

CopyHostedDocumentOperation::CopyHostedDocumentOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const GetDataCallback& callback,
    const std::string& resource_id,
    const std::string& new_name)
    : GetDataOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(resource_id),
      new_name_(new_name) {
  DCHECK(!callback.is_null());
}

CopyHostedDocumentOperation::~CopyHostedDocumentOperation() {}

URLFetcher::RequestType CopyHostedDocumentOperation::GetRequestType() const {
  return URLFetcher::POST;
}

GURL CopyHostedDocumentOperation::GetURL() const {
  return url_generator_.GenerateResourceListRootUrl();
}

bool CopyHostedDocumentOperation::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content) {
  upload_content_type->assign("application/atom+xml");
  XmlWriter xml_writer;
  xml_writer.StartWriting();
  xml_writer.StartElement("entry");
  xml_writer.AddAttribute("xmlns", "http://www.w3.org/2005/Atom");

  xml_writer.WriteElement("id", resource_id_);
  xml_writer.WriteElement("title", new_name_);

  xml_writer.EndElement();  // Ends "entry" element.
  xml_writer.StopWriting();
  upload_content->assign(xml_writer.GetWrittenString());
  DVLOG(1) << "CopyHostedDocumentOperation data: " << *upload_content_type
           << ", [" << *upload_content << "]";
  return true;
}

//=========================== RenameResourceOperation ==========================

RenameResourceOperation::RenameResourceOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const EntryActionCallback& callback,
    const std::string& resource_id,
    const std::string& new_name)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(resource_id),
      new_name_(new_name) {
  DCHECK(!callback.is_null());
}

RenameResourceOperation::~RenameResourceOperation() {}

URLFetcher::RequestType RenameResourceOperation::GetRequestType() const {
  return URLFetcher::PUT;
}

std::vector<std::string>
RenameResourceOperation::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(kIfMatchAllHeader);
  return headers;
}

GURL RenameResourceOperation::GetURL() const {
  return url_generator_.GenerateEditUrl(resource_id_);
}

bool RenameResourceOperation::GetContentData(std::string* upload_content_type,
                                             std::string* upload_content) {
  upload_content_type->assign("application/atom+xml");
  XmlWriter xml_writer;
  xml_writer.StartWriting();
  xml_writer.StartElement("entry");
  xml_writer.AddAttribute("xmlns", "http://www.w3.org/2005/Atom");

  xml_writer.WriteElement("title", new_name_);

  xml_writer.EndElement();  // Ends "entry" element.
  xml_writer.StopWriting();
  upload_content->assign(xml_writer.GetWrittenString());
  DVLOG(1) << "RenameResourceOperation data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//=========================== AuthorizeAppOperation ==========================

AuthorizeAppOperation::AuthorizeAppOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GetDataCallback& callback,
    const GURL& edit_url,
    const std::string& app_id)
    : GetDataOperation(registry, url_request_context_getter, callback),
      app_id_(app_id),
      edit_url_(edit_url) {
  DCHECK(!callback.is_null());
}

AuthorizeAppOperation::~AuthorizeAppOperation() {}

URLFetcher::RequestType AuthorizeAppOperation::GetRequestType() const {
  return URLFetcher::PUT;
}

std::vector<std::string>
AuthorizeAppOperation::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(kIfMatchAllHeader);
  return headers;
}

bool AuthorizeAppOperation::GetContentData(std::string* upload_content_type,
                                            std::string* upload_content) {
  upload_content_type->assign("application/atom+xml");
  XmlWriter xml_writer;
  xml_writer.StartWriting();
  xml_writer.StartElement("entry");
  xml_writer.AddAttribute("xmlns", "http://www.w3.org/2005/Atom");
  xml_writer.AddAttribute("xmlns:docs", "http://schemas.google.com/docs/2007");
  xml_writer.WriteElement("docs:authorizedApp", app_id_);

  xml_writer.EndElement();  // Ends "entry" element.
  xml_writer.StopWriting();
  upload_content->assign(xml_writer.GetWrittenString());
  DVLOG(1) << "AuthorizeAppOperation data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

GURL AuthorizeAppOperation::GetURL() const {
  return GDataWapiUrlGenerator::AddStandardUrlParams(edit_url_);
}

//======================= AddResourceToDirectoryOperation ======================

AddResourceToDirectoryOperation::AddResourceToDirectoryOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const EntryActionCallback& callback,
    const std::string& parent_resource_id,
    const std::string& resource_id)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      resource_id_(resource_id) {
  DCHECK(!callback.is_null());
}

AddResourceToDirectoryOperation::~AddResourceToDirectoryOperation() {}

GURL AddResourceToDirectoryOperation::GetURL() const {
  return url_generator_.GenerateContentUrl(parent_resource_id_);
}

URLFetcher::RequestType
AddResourceToDirectoryOperation::GetRequestType() const {
  return URLFetcher::POST;
}

bool AddResourceToDirectoryOperation::GetContentData(
    std::string* upload_content_type, std::string* upload_content) {
  upload_content_type->assign("application/atom+xml");
  XmlWriter xml_writer;
  xml_writer.StartWriting();
  xml_writer.StartElement("entry");
  xml_writer.AddAttribute("xmlns", "http://www.w3.org/2005/Atom");

  xml_writer.WriteElement(
      "id", url_generator_.GenerateEditUrlWithoutParams(resource_id_).spec());

  xml_writer.EndElement();  // Ends "entry" element.
  xml_writer.StopWriting();
  upload_content->assign(xml_writer.GetWrittenString());
  DVLOG(1) << "AddResourceToDirectoryOperation data: " << *upload_content_type
           << ", [" << *upload_content << "]";
  return true;
}

//==================== RemoveResourceFromDirectoryOperation ====================

RemoveResourceFromDirectoryOperation::RemoveResourceFromDirectoryOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const EntryActionCallback& callback,
    const std::string& parent_resource_id,
    const std::string& document_resource_id)
    : EntryActionOperation(registry, url_request_context_getter, callback),
      url_generator_(url_generator),
      resource_id_(document_resource_id),
      parent_resource_id_(parent_resource_id) {
  DCHECK(!callback.is_null());
}

RemoveResourceFromDirectoryOperation::~RemoveResourceFromDirectoryOperation() {
}

GURL RemoveResourceFromDirectoryOperation::GetURL() const {
  return url_generator_.GenerateResourceUrlForRemoval(
      parent_resource_id_, resource_id_);
}

URLFetcher::RequestType
RemoveResourceFromDirectoryOperation::GetRequestType() const {
  return URLFetcher::DELETE_REQUEST;
}

std::vector<std::string>
RemoveResourceFromDirectoryOperation::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(kIfMatchAllHeader);
  return headers;
}

//======================= InitiateUploadNewFileOperation =======================

InitiateUploadNewFileOperation::InitiateUploadNewFileOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const InitiateUploadCallback& callback,
    const base::FilePath& drive_file_path,
    const std::string& content_type,
    int64 content_length,
    const std::string& parent_resource_id,
    const std::string& title)
    : InitiateUploadOperationBase(registry,
                                  url_request_context_getter,
                                  callback,
                                  drive_file_path,
                                  content_type,
                                  content_length),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      title_(title) {
}

InitiateUploadNewFileOperation::~InitiateUploadNewFileOperation() {}

GURL InitiateUploadNewFileOperation::GetURL() const {
  return url_generator_.GenerateInitiateUploadNewFileUrl(parent_resource_id_);
}

net::URLFetcher::RequestType
InitiateUploadNewFileOperation::GetRequestType() const {
  return net::URLFetcher::POST;
}

bool InitiateUploadNewFileOperation::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content) {
  upload_content_type->assign("application/atom+xml");
  XmlWriter xml_writer;
  xml_writer.StartWriting();
  xml_writer.StartElement("entry");
  xml_writer.AddAttribute("xmlns", "http://www.w3.org/2005/Atom");
  xml_writer.AddAttribute("xmlns:docs",
                          "http://schemas.google.com/docs/2007");
  xml_writer.WriteElement("title", title_);
  xml_writer.EndElement();  // Ends "entry" element.
  xml_writer.StopWriting();
  upload_content->assign(xml_writer.GetWrittenString());
  DVLOG(1) << "InitiateUploadNewFile: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//===================== InitiateUploadExistingFileOperation ====================

InitiateUploadExistingFileOperation::InitiateUploadExistingFileOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const GDataWapiUrlGenerator& url_generator,
    const InitiateUploadCallback& callback,
    const base::FilePath& drive_file_path,
    const std::string& content_type,
    int64 content_length,
    const std::string& resource_id,
    const std::string& etag)
    : InitiateUploadOperationBase(registry,
                                  url_request_context_getter,
                                  callback,
                                  drive_file_path,
                                  content_type,
                                  content_length),
      url_generator_(url_generator),
      resource_id_(resource_id),
      etag_(etag) {
}

InitiateUploadExistingFileOperation::~InitiateUploadExistingFileOperation() {}

GURL InitiateUploadExistingFileOperation::GetURL() const {
  return url_generator_.GenerateInitiateUploadExistingFileUrl(resource_id_);
}

net::URLFetcher::RequestType
InitiateUploadExistingFileOperation::GetRequestType() const {
  return net::URLFetcher::PUT;
}

bool InitiateUploadExistingFileOperation::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content) {
  // According to the document there is no need to send the content-type.
  // However, the server would return 500 server error without the
  // content-type.
  // As its workaround, send "text/plain" content-type here.
  *upload_content_type = "text/plain";
  *upload_content = "";
  return true;
}

std::vector<std::string>
InitiateUploadExistingFileOperation::GetExtraRequestHeaders() const {
  std::vector<std::string> headers(
      InitiateUploadOperationBase::GetExtraRequestHeaders());
  headers.push_back(GenerateIfMatchHeader(etag_));
  return headers;
}

//============================ ResumeUploadOperation ===========================

ResumeUploadOperation::ResumeUploadOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const UploadRangeCallback& callback,
    const ResumeUploadParams& params)
  : UploadRangeOperationBase(registry,
                             url_request_context_getter,
                             params.upload_mode,
                             params.drive_file_path,
                             params.upload_location),
    callback_(callback),
    start_position_(params.start_position),
    end_position_(params.end_position),
    content_length_(params.content_length),
    content_type_(params.content_type),
    buf_(params.buf) {
  DCHECK(!callback_.is_null());
  DCHECK_LE(start_position_, end_position_);
}

ResumeUploadOperation::~ResumeUploadOperation() {}

std::vector<std::string> ResumeUploadOperation::GetExtraRequestHeaders() const {
  if (content_length_ == 0) {
    // For uploading an empty document, just PUT an empty content.
    DCHECK_EQ(start_position_, 0);
    DCHECK_EQ(end_position_, 0);
    return std::vector<std::string>();
  }

  // The header looks like
  // Content-Range: bytes <start_position>-<end_position>/<content_length>
  // for example:
  // Content-Range: bytes 7864320-8388607/13851821
  // The header takes inclusive range, so we adjust by "end_position - 1".
  DCHECK_GE(start_position_, 0);
  DCHECK_GT(end_position_, 0);
  DCHECK_GE(content_length_, 0);

  std::vector<std::string> headers;
  headers.push_back(
      std::string(kUploadContentRange) +
      base::Int64ToString(start_position_) + "-" +
      base::Int64ToString(end_position_ - 1) + "/" +
      base::Int64ToString(content_length_));
  return headers;
}

bool ResumeUploadOperation::GetContentData(std::string* upload_content_type,
                                           std::string* upload_content) {
  *upload_content_type = content_type_;
  *upload_content = std::string(buf_->data(), end_position_ - start_position_);
  return true;
}

void ResumeUploadOperation::OnURLFetchUploadProgress(
    const URLFetcher* source, int64 current, int64 total) {
  // Adjust the progress values according to the range currently uploaded.
  NotifyProgress(start_position_ + current, content_length_);
}

void ResumeUploadOperation::OnRangeOperationComplete(
    const UploadRangeResponse& response, scoped_ptr<base::Value> value) {
  callback_.Run(response, ParseResourceEntry(value.Pass()));
}

//========================== GetUploadStatusOperation ==========================

GetUploadStatusOperation::GetUploadStatusOperation(
    OperationRegistry* registry,
    net::URLRequestContextGetter* url_request_context_getter,
    const UploadRangeCallback& callback,
    UploadMode upload_mode,
    const base::FilePath& drive_file_path,
    const GURL& upload_url,
    int64 content_length)
  : UploadRangeOperationBase(registry,
                             url_request_context_getter,
                             upload_mode,
                             drive_file_path,
                             upload_url),
    callback_(callback),
    content_length_(content_length) {}

GetUploadStatusOperation::~GetUploadStatusOperation() {}

std::vector<std::string>
GetUploadStatusOperation::GetExtraRequestHeaders() const {
  // The header looks like
  // Content-Range: bytes */<content_length>
  // for example:
  // Content-Range: bytes */13851821
  DCHECK_GE(content_length_, 0);

  std::vector<std::string> headers;
  headers.push_back(
      std::string(kUploadContentRange) + "*/" +
      base::Int64ToString(content_length_));
  return headers;
}

void GetUploadStatusOperation::OnRangeOperationComplete(
    const UploadRangeResponse& response, scoped_ptr<base::Value> value) {
  callback_.Run(response, ParseResourceEntry(value.Pass()));
}

}  // namespace google_apis
