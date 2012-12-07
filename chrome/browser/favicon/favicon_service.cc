// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/favicon/favicon_service.h"

#include "base/message_loop_proxy.h"
#include "chrome/browser/favicon/favicon_util.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/history_backend.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/select_favicon_frames.h"
#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "chrome/common/url_constants.h"
#include "extensions/common/constants.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/image/image_skia.h"

using base::Bind;

namespace {

void CancelOrRunFaviconResultsCallback(
    const CancelableTaskTracker::IsCanceledCallback& is_canceled,
    const FaviconService::FaviconResultsCallback2& callback,
    const std::vector<history::FaviconBitmapResult>& results,
    const history::IconURLSizesMap& size_map) {
  if (is_canceled.Run())
    return;
  callback.Run(results, size_map);
}

}  // namespace

FaviconService::FaviconService(HistoryService* history_service)
    : history_service_(history_service) {
}

// static
void FaviconService::FaviconResultsCallbackRunner(
    const FaviconResultsCallback2& callback,
    const std::vector<history::FaviconBitmapResult>* results,
    const history::IconURLSizesMap* size_map) {
  callback.Run(*results, *size_map);
}

FaviconService::Handle FaviconService::GetFaviconImage(
    const GURL& icon_url,
    history::IconType icon_type,
    int desired_size_in_dip,
    CancelableRequestConsumerBase* consumer,
    const FaviconImageCallback& callback) {
  GetFaviconRequest* request = new GetFaviconRequest(
      Bind(&FaviconService::RunFaviconImageCallbackWithBitmapResults,
           base::Unretained(this),
           callback,
           desired_size_in_dip));
  AddRequest(request, consumer);
  if (history_service_) {
    std::vector<GURL> icon_urls;
    icon_urls.push_back(icon_url);
    history_service_->GetFavicons(request, icon_urls, icon_type,
        desired_size_in_dip, ui::GetSupportedScaleFactors());
  } else {
    ForwardEmptyResultAsync(request);
  }
  return request->handle();
}

FaviconService::Handle FaviconService::GetRawFavicon(
    const GURL& icon_url,
    history::IconType icon_type,
    int desired_size_in_dip,
    ui::ScaleFactor desired_scale_factor,
    CancelableRequestConsumerBase* consumer,
    const FaviconRawCallback& callback) {
  GetFaviconRequest* request = new GetFaviconRequest(
      Bind(&FaviconService::RunFaviconRawCallbackWithBitmapResults,
           base::Unretained(this),
           callback,
           desired_size_in_dip,
           desired_scale_factor));
  AddRequest(request, consumer);
  if (history_service_) {
    std::vector<GURL> icon_urls;
    icon_urls.push_back(icon_url);
    std::vector<ui::ScaleFactor> desired_scale_factors;
    desired_scale_factors.push_back(desired_scale_factor);
    history_service_->GetFavicons(request, icon_urls, icon_type,
        desired_size_in_dip, desired_scale_factors);
  } else {
    ForwardEmptyResultAsync(request);
  }
  return request->handle();
}

FaviconService::Handle FaviconService::GetFavicon(
    const GURL& icon_url,
    history::IconType icon_type,
    int desired_size_in_dip,
    CancelableRequestConsumerBase* consumer,
    const FaviconResultsCallback& callback) {
  GetFaviconRequest* request = new GetFaviconRequest(callback);
  AddRequest(request, consumer);
  if (history_service_) {
    std::vector<GURL> icon_urls;
    icon_urls.push_back(icon_url);
    history_service_->GetFavicons(request, icon_urls, icon_type,
        desired_size_in_dip, FaviconUtil::GetFaviconScaleFactors());
  } else {
    ForwardEmptyResultAsync(request);
  }
  return request->handle();
}

FaviconService::Handle FaviconService::UpdateFaviconMappingsAndFetch(
    const GURL& page_url,
    const std::vector<GURL>& icon_urls,
    int icon_types,
    int desired_size_in_dip,
    CancelableRequestConsumerBase* consumer,
    const FaviconResultsCallback& callback) {
  GetFaviconRequest* request = new GetFaviconRequest(callback);
  AddRequest(request, consumer);
  if (history_service_) {
    history_service_->UpdateFaviconMappingsAndFetch(request, page_url,
        icon_urls, icon_types, desired_size_in_dip,
        FaviconUtil::GetFaviconScaleFactors());
  } else {
    ForwardEmptyResultAsync(request);
  }
  return request->handle();
}

CancelableTaskTracker::TaskId FaviconService::GetFaviconImageForURL(
    const FaviconForURLParams& params,
    const FaviconImageCallback2& callback,
    CancelableTaskTracker* tracker) {
  return GetFaviconForURLImpl(
      params,
      FaviconUtil::GetFaviconScaleFactors(),
      Bind(&FaviconService::RunFaviconImageCallbackWithBitmapResults2,
           base::Unretained(this),
           callback,
           params.desired_size_in_dip),
      tracker);
}

CancelableTaskTracker::TaskId FaviconService::GetRawFaviconForURL(
    const FaviconForURLParams& params,
    ui::ScaleFactor desired_scale_factor,
    const FaviconRawCallback2& callback,
    CancelableTaskTracker* tracker) {
  std::vector<ui::ScaleFactor> desired_scale_factors;
  desired_scale_factors.push_back(desired_scale_factor);
  return GetFaviconForURLImpl(
      params,
      desired_scale_factors,
      Bind(&FaviconService::RunFaviconRawCallbackWithBitmapResults2,
           base::Unretained(this),
           callback,
           params.desired_size_in_dip,
           desired_scale_factor),
      tracker);
}

CancelableTaskTracker::TaskId FaviconService::GetFaviconForURL(
    const FaviconForURLParams& params,
    const FaviconResultsCallback2& callback,
    CancelableTaskTracker* tracker) {
  return GetFaviconForURLImpl(params,
                              FaviconUtil::GetFaviconScaleFactors(),
                              callback,
                              tracker);
}

FaviconService::Handle FaviconService::GetLargestRawFaviconForID(
      history::FaviconID favicon_id,
      CancelableRequestConsumerBase* consumer,
      const FaviconRawCallback& callback) {
   // Use 0 as |desired_size_in_dip| to get the largest bitmap for |favicon_id|
   // without any resizing.
   int desired_size_in_dip = 0;
   ui::ScaleFactor desired_scale_factor = ui::SCALE_FACTOR_100P;
   GetFaviconRequest* request = new GetFaviconRequest(
       Bind(&FaviconService::RunFaviconRawCallbackWithBitmapResults,
            base::Unretained(this),
            callback,
            desired_size_in_dip,
            desired_scale_factor));

  AddRequest(request, consumer);
  FaviconService::Handle handle = request->handle();
  if (history_service_) {
    history_service_->GetFaviconForID(request, favicon_id, desired_size_in_dip,
                                      desired_scale_factor);
  } else {
    ForwardEmptyResultAsync(request);
  }
  return handle;
}

void FaviconService::SetFaviconOutOfDateForPage(const GURL& page_url) {
  if (history_service_)
    history_service_->SetFaviconsOutOfDateForPage(page_url);
}

void FaviconService::CloneFavicon(const GURL& old_page_url,
                                  const GURL& new_page_url) {
  if (history_service_)
    history_service_->CloneFavicons(old_page_url, new_page_url);
}

void FaviconService::SetImportedFavicons(
    const std::vector<history::ImportedFaviconUsage>& favicon_usage) {
  if (history_service_)
    history_service_->SetImportedFavicons(favicon_usage);
}

void FaviconService::MergeFavicon(
    const GURL& page_url,
    const GURL& icon_url,
    history::IconType icon_type,
    scoped_refptr<base::RefCountedMemory> bitmap_data,
    const gfx::Size& pixel_size) {
  if (history_service_) {
    history_service_->MergeFavicon(page_url, icon_url, icon_type, bitmap_data,
                                   pixel_size);
  }
}

void FaviconService::SetFavicons(
    const GURL& page_url,
    const GURL& icon_url,
    history::IconType icon_type,
    const gfx::Image& image) {
  if (!history_service_)
    return;

  gfx::ImageSkia image_skia = image.AsImageSkia();
  image_skia.EnsureRepsForSupportedScaleFactors();
  const std::vector<gfx::ImageSkiaRep>& image_reps = image_skia.image_reps();
  std::vector<history::FaviconBitmapData> favicon_bitmap_data;
  history::FaviconSizes favicon_sizes;
  for (size_t i = 0; i < image_reps.size(); ++i) {
    scoped_refptr<base::RefCountedBytes> bitmap_data(
        new base::RefCountedBytes());
    if (gfx::PNGCodec::EncodeBGRASkBitmap(image_reps[i].sk_bitmap(),
                                          false,
                                          &bitmap_data->data())) {
      gfx::Size pixel_size(image_reps[i].pixel_width(),
                           image_reps[i].pixel_height());
      history::FaviconBitmapData bitmap_data_element;
      bitmap_data_element.bitmap_data = bitmap_data;
      bitmap_data_element.pixel_size = pixel_size;
      bitmap_data_element.icon_url = icon_url;

      favicon_bitmap_data.push_back(bitmap_data_element);

      // Construct favicon sizes from a guess at what the HTML 5 'sizes'
      // attribute in the link tag is.
      // TODO(pkotwicz): Plumb the HTML 5 sizes attribute to FaviconHandler.
      favicon_sizes.push_back(pixel_size);
    }
  }

  // TODO(pkotwicz): Tell the database about all the icon URLs associated
  // with |page_url|.
  history::IconURLSizesMap icon_url_sizes;
  icon_url_sizes[icon_url] = favicon_sizes;

  history_service_->SetFavicons(page_url, icon_type, favicon_bitmap_data,
      icon_url_sizes);
}

FaviconService::~FaviconService() {}

CancelableTaskTracker::TaskId FaviconService::GetFaviconForURLImpl(
    const FaviconForURLParams& params,
    const std::vector<ui::ScaleFactor>& desired_scale_factors,
    const FaviconResultsCallback2& callback,
    CancelableTaskTracker* tracker) {
  if (params.page_url.SchemeIs(chrome::kChromeUIScheme) ||
      params.page_url.SchemeIs(extensions::kExtensionScheme)) {
    CancelableTaskTracker::IsCanceledCallback is_canceled_cb;
    CancelableTaskTracker::TaskId id =
        tracker->NewTrackedTaskId(&is_canceled_cb);

    FaviconResultsCallback2 cancelable_cb =
        Bind(&CancelOrRunFaviconResultsCallback, is_canceled_cb, callback);
    ChromeWebUIControllerFactory::GetInstance()->GetFaviconForURL(
        params.profile, params.page_url, desired_scale_factors, cancelable_cb);
    return id;
  } else if (history_service_) {
    return history_service_->GetFaviconsForURL(params.page_url,
                                               params.icon_types,
                                               params.desired_size_in_dip,
                                               desired_scale_factors,
                                               callback,
                                               tracker);
  } else {
    return RunWithEmptyResultAsync(callback, tracker);
  }
}

void FaviconService::RunFaviconImageCallbackWithBitmapResults(
    FaviconImageCallback callback,
    int desired_size_in_dip,
    Handle handle,
    std::vector<history::FaviconBitmapResult> favicon_bitmap_results,
    history::IconURLSizesMap icon_url_sizes_map) {
  history::FaviconImageResult image_result;
  image_result.image = FaviconUtil::SelectFaviconFramesFromPNGs(
      favicon_bitmap_results,
      FaviconUtil::GetFaviconScaleFactors(),
      desired_size_in_dip);
  image_result.icon_url = image_result.image.IsEmpty() ?
      GURL() : favicon_bitmap_results[0].icon_url;
  callback.Run(handle, image_result);
}

void FaviconService::RunFaviconImageCallbackWithBitmapResults2(
    const FaviconImageCallback2& callback,
    int desired_size_in_dip,
    const std::vector<history::FaviconBitmapResult>& favicon_bitmap_results,
    const history::IconURLSizesMap& icon_url_sizes_map) {
  history::FaviconImageResult image_result;
  image_result.image = FaviconUtil::SelectFaviconFramesFromPNGs(
      favicon_bitmap_results,
      FaviconUtil::GetFaviconScaleFactors(),
      desired_size_in_dip);
  image_result.icon_url = image_result.image.IsEmpty() ?
      GURL() : favicon_bitmap_results[0].icon_url;
  callback.Run(image_result);
}

void FaviconService::RunFaviconRawCallbackWithBitmapResults(
    FaviconRawCallback callback,
    int desired_size_in_dip,
    ui::ScaleFactor desired_scale_factor,
    Handle handle,
    std::vector<history::FaviconBitmapResult> favicon_bitmap_results,
    history::IconURLSizesMap icon_url_sizes_map) {
  if (favicon_bitmap_results.empty() || !favicon_bitmap_results[0].is_valid()) {
    callback.Run(handle, history::FaviconBitmapResult());
    return;
  }

  DCHECK_EQ(1u, favicon_bitmap_results.size());
  history::FaviconBitmapResult bitmap_result = favicon_bitmap_results[0];

  // If the desired size is 0, SelectFaviconFrames() will return the largest
  // bitmap without doing any resizing. As |favicon_bitmap_results| has bitmap
  // data for a single bitmap, return it and avoid an unnecessary decode.
  if (desired_size_in_dip == 0) {
    callback.Run(handle, bitmap_result);
    return;
  }

  // If history bitmap is already desired pixel size, return early.
  float desired_scale = ui::GetScaleFactorScale(desired_scale_factor);
  int desired_edge_width_in_pixel = static_cast<int>(
      desired_size_in_dip * desired_scale + 0.5f);
  gfx::Size desired_size_in_pixel(desired_edge_width_in_pixel,
                                  desired_edge_width_in_pixel);
  if (bitmap_result.pixel_size == desired_size_in_pixel) {
    callback.Run(handle, bitmap_result);
    return;
  }

  // Convert raw bytes to SkBitmap, resize via SelectFaviconFrames(), then
  // convert back.
  std::vector<ui::ScaleFactor> desired_scale_factors;
  desired_scale_factors.push_back(desired_scale_factor);
  gfx::Image resized_image = FaviconUtil::SelectFaviconFramesFromPNGs(
      favicon_bitmap_results, desired_scale_factors, desired_size_in_dip);

  std::vector<unsigned char> resized_bitmap_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(resized_image.AsBitmap(), false,
                                         &resized_bitmap_data)) {
    callback.Run(handle, history::FaviconBitmapResult());
    return;
  }

  bitmap_result.bitmap_data = base::RefCountedBytes::TakeVector(
      &resized_bitmap_data);
  callback.Run(handle, bitmap_result);
}

void FaviconService::RunFaviconRawCallbackWithBitmapResults2(
    const FaviconRawCallback2& callback,
    int desired_size_in_dip,
    ui::ScaleFactor desired_scale_factor,
    const std::vector<history::FaviconBitmapResult>& favicon_bitmap_results,
    const history::IconURLSizesMap& icon_url_sizes_map) {
  if (favicon_bitmap_results.empty() || !favicon_bitmap_results[0].is_valid()) {
    callback.Run(history::FaviconBitmapResult());
    return;
  }

  DCHECK_EQ(1u, favicon_bitmap_results.size());
  history::FaviconBitmapResult bitmap_result = favicon_bitmap_results[0];

  // If the desired size is 0, SelectFaviconFrames() will return the largest
  // bitmap without doing any resizing. As |favicon_bitmap_results| has bitmap
  // data for a single bitmap, return it and avoid an unnecessary decode.
  if (desired_size_in_dip == 0) {
    callback.Run(bitmap_result);
    return;
  }

  // If history bitmap is already desired pixel size, return early.
  float desired_scale = ui::GetScaleFactorScale(desired_scale_factor);
  int desired_edge_width_in_pixel = static_cast<int>(
      desired_size_in_dip * desired_scale + 0.5f);
  gfx::Size desired_size_in_pixel(desired_edge_width_in_pixel,
                                  desired_edge_width_in_pixel);
  if (bitmap_result.pixel_size == desired_size_in_pixel) {
    callback.Run(bitmap_result);
    return;
  }

  // Convert raw bytes to SkBitmap, resize via SelectFaviconFrames(), then
  // convert back.
  std::vector<ui::ScaleFactor> desired_scale_factors;
  desired_scale_factors.push_back(desired_scale_factor);
  gfx::Image resized_image = FaviconUtil::SelectFaviconFramesFromPNGs(
      favicon_bitmap_results, desired_scale_factors, desired_size_in_dip);

  std::vector<unsigned char> resized_bitmap_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(resized_image.AsBitmap(), false,
                                         &resized_bitmap_data)) {
    callback.Run(history::FaviconBitmapResult());
    return;
  }

  bitmap_result.bitmap_data = base::RefCountedBytes::TakeVector(
      &resized_bitmap_data);
  callback.Run(bitmap_result);
}

void FaviconService::ForwardEmptyResultAsync(GetFaviconRequest* request) {
  request->ForwardResultAsync(request->handle(),
                              std::vector<history::FaviconBitmapResult>(),
                              history::IconURLSizesMap());
}

CancelableTaskTracker::TaskId FaviconService::RunWithEmptyResultAsync(
    const FaviconResultsCallback2& callback,
    CancelableTaskTracker* tracker) {
  return tracker->PostTask(
      base::MessageLoopProxy::current(),
      FROM_HERE,
      Bind(callback,
           std::vector<history::FaviconBitmapResult>(),
           history::IconURLSizesMap()));
}
