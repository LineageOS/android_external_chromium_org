// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/ppb_flash_impl.h"

#include <string>
#include <vector>

#include "base/message_loop.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "googleurl/src/gurl.h"
#include "ppapi/c/dev/ppb_font_dev.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/shared_impl/time_conversion.h"
#include "ppapi/shared_impl/var.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkTemplates.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "ui/gfx/rect.h"
#include "webkit/glue/clipboard_client.h"
#include "webkit/glue/scoped_clipboard_writer_glue.h"
#include "webkit/plugins/ppapi/common.h"
#include "webkit/plugins/ppapi/host_globals.h"
#include "webkit/plugins/ppapi/plugin_delegate.h"
#include "webkit/plugins/ppapi/plugin_module.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"
#include "webkit/plugins/ppapi/ppb_url_request_info_impl.h"
#include "webkit/plugins/ppapi/resource_helper.h"
#include "webkit/plugins/ppapi/ppb_image_data_impl.h"

using ppapi::PPTimeToTime;
using ppapi::StringVar;
using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_ImageData_API;
using ppapi::thunk::PPB_URLRequestInfo_API;

namespace webkit {
namespace ppapi {

namespace {

const size_t kMaxClipboardWriteSize = 1000000;

ui::Clipboard::Buffer ConvertClipboardType(
    PP_Flash_Clipboard_Type type) {
  switch (type) {
    case PP_FLASH_CLIPBOARD_TYPE_STANDARD:
      return ui::Clipboard::BUFFER_STANDARD;
    case PP_FLASH_CLIPBOARD_TYPE_SELECTION:
      return ui::Clipboard::BUFFER_SELECTION;
  }
  NOTREACHED();
  return ui::Clipboard::BUFFER_STANDARD;
}

}  // namespace

PPB_Flash_Impl::PPB_Flash_Impl(PluginInstance* instance)
    : instance_(instance) {
}

PPB_Flash_Impl::~PPB_Flash_Impl() {
}

void PPB_Flash_Impl::SetInstanceAlwaysOnTop(PP_Instance instance,
                                            PP_Bool on_top) {
  instance_->set_always_on_top(PP_ToBool(on_top));
}

PP_Bool PPB_Flash_Impl::DrawGlyphs(PP_Instance instance,
                                   PP_Resource pp_image_data,
                                   const PP_FontDescription_Dev* font_desc,
                                   uint32_t color,
                                   const PP_Point* position,
                                   const PP_Rect* clip,
                                   const float transformation[3][3],
                                   PP_Bool allow_subpixel_aa,
                                   uint32_t glyph_count,
                                   const uint16_t glyph_indices[],
                                   const PP_Point glyph_advances[]) {
  EnterResourceNoLock<PPB_ImageData_API> enter(pp_image_data, true);
  if (enter.failed())
    return PP_FALSE;
  PPB_ImageData_Impl* image_resource =
      static_cast<PPB_ImageData_Impl*>(enter.object());

  ImageDataAutoMapper mapper(image_resource);
  if (!mapper.is_valid())
    return PP_FALSE;

  // Set up the typeface.
  StringVar* face_name = StringVar::FromPPVar(font_desc->face);
  if (!face_name)
    return PP_FALSE;
  int style = SkTypeface::kNormal;
  if (font_desc->weight >= PP_FONTWEIGHT_BOLD)
    style |= SkTypeface::kBold;
  if (font_desc->italic)
    style |= SkTypeface::kItalic;
  SkTypeface* typeface =
      SkTypeface::CreateFromName(face_name->value().c_str(),
                                 static_cast<SkTypeface::Style>(style));
  if (!typeface)
    return PP_FALSE;
  SkAutoUnref aur(typeface);

  // Set up the canvas.
  SkCanvas* canvas = image_resource->GetPlatformCanvas();
  SkAutoCanvasRestore acr(canvas, true);

  // Clip is applied in pixels before the transform.
  SkRect clip_rect = { clip->point.x, clip->point.y,
                       clip->point.x + clip->size.width,
                       clip->point.y + clip->size.height };
  canvas->clipRect(clip_rect);

  // Convert & set the matrix.
  SkMatrix matrix;
  matrix.set(SkMatrix::kMScaleX, SkFloatToScalar(transformation[0][0]));
  matrix.set(SkMatrix::kMSkewX,  SkFloatToScalar(transformation[0][1]));
  matrix.set(SkMatrix::kMTransX, SkFloatToScalar(transformation[0][2]));
  matrix.set(SkMatrix::kMSkewY,  SkFloatToScalar(transformation[1][0]));
  matrix.set(SkMatrix::kMScaleY, SkFloatToScalar(transformation[1][1]));
  matrix.set(SkMatrix::kMTransY, SkFloatToScalar(transformation[1][2]));
  matrix.set(SkMatrix::kMPersp0, SkFloatToScalar(transformation[2][0]));
  matrix.set(SkMatrix::kMPersp1, SkFloatToScalar(transformation[2][1]));
  matrix.set(SkMatrix::kMPersp2, SkFloatToScalar(transformation[2][2]));
  canvas->concat(matrix);

  SkPaint paint;
  paint.setColor(color);
  paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  paint.setAntiAlias(true);
  paint.setHinting(SkPaint::kFull_Hinting);
  paint.setTextSize(SkIntToScalar(font_desc->size));
  paint.setTypeface(typeface);  // Takes a ref and manages lifetime.
  if (allow_subpixel_aa) {
    paint.setSubpixelText(true);
    paint.setLCDRenderText(true);
  }

  SkScalar x = SkIntToScalar(position->x);
  SkScalar y = SkIntToScalar(position->y);

  // Build up the skia advances.
  if (glyph_count == 0)
    return PP_TRUE;
  std::vector<SkPoint> storage;
  storage.resize(glyph_count);
  SkPoint* sk_positions = &storage[0];
  for (uint32_t i = 0; i < glyph_count; i++) {
    sk_positions[i].set(x, y);
    x += SkFloatToScalar(glyph_advances[i].x);
    y += SkFloatToScalar(glyph_advances[i].y);
  }

  canvas->drawPosText(glyph_indices, glyph_count * 2, sk_positions, paint);

  return PP_TRUE;
}

PP_Var PPB_Flash_Impl::GetProxyForURL(PP_Instance instance,
                                      const char* url) {
  GURL gurl(url);
  if (!gurl.is_valid())
    return PP_MakeUndefined();

  std::string proxy_host = instance_->delegate()->ResolveProxy(gurl);
  if (proxy_host.empty())
    return PP_MakeUndefined();  // No proxy.
  return StringVar::StringToPPVar(proxy_host);
}

int32_t PPB_Flash_Impl::Navigate(PP_Instance instance,
                                 PP_Resource request_info,
                                 const char* target,
                                 PP_Bool from_user_action) {
  EnterResourceNoLock<PPB_URLRequestInfo_API> enter(request_info, true);
  if (enter.failed())
    return PP_ERROR_BADRESOURCE;
  PPB_URLRequestInfo_Impl* request =
      static_cast<PPB_URLRequestInfo_Impl*>(enter.object());

  if (!target)
    return PP_ERROR_BADARGUMENT;
  return instance_->Navigate(request, target, PP_ToBool(from_user_action));
}

void PPB_Flash_Impl::RunMessageLoop(PP_Instance instance) {
  MessageLoop::ScopedNestableTaskAllower allow(MessageLoop::current());
  MessageLoop::current()->Run();
}

void PPB_Flash_Impl::QuitMessageLoop(PP_Instance instance) {
  MessageLoop::current()->QuitNow();
}

double PPB_Flash_Impl::GetLocalTimeZoneOffset(PP_Instance instance,
                                              PP_Time t) {
  // Evil hack. The time code handles exact "0" values as special, and produces
  // a "null" Time object. This will represent some date hundreds of years ago
  // and will give us funny results at 1970 (there are some tests where this
  // comes up, but it shouldn't happen in real life). To work around this
  // special handling, we just need to give it some nonzero value.
  if (t == 0.0)
    t = 0.0000000001;

  // We can't do the conversion here because on Linux, the localtime calls
  // require filesystem access prohibited by the sandbox.
  return instance_->delegate()->GetLocalTimeZoneOffset(PPTimeToTime(t));
}

PP_Bool PPB_Flash_Impl::IsRectTopmost(PP_Instance instance,
                                      const PP_Rect* rect) {
  return PP_FromBool(instance_->IsRectTopmost(
      gfx::Rect(rect->point.x, rect->point.y,
                rect->size.width, rect->size.height)));
}

int32_t PPB_Flash_Impl::InvokePrinting(PP_Instance instance) {
  // TODO(viettrungluu): Implement me.
  return PP_ERROR_NOTSUPPORTED;
}

void PPB_Flash_Impl::UpdateActivity(PP_Instance pp_instance) {
  // Not supported in-process.
}

PP_Var PPB_Flash_Impl::GetDeviceID(PP_Instance pp_instance) {
  // Not supported in-process.
  return PP_MakeUndefined();
}

PP_Bool PPB_Flash_Impl::IsClipboardFormatAvailable(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    PP_Flash_Clipboard_Format format) {
  if (!InitClipboard())
    return PP_FALSE;

  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_FALSE;
  }

  ui::Clipboard::Buffer buffer_type = ConvertClipboardType(clipboard_type);
  switch (format) {
    case PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT: {
      bool plain = clipboard_client_->IsFormatAvailable(
          ui::Clipboard::GetPlainTextFormatType(), buffer_type);
      bool plainw = clipboard_client_->IsFormatAvailable(
          ui::Clipboard::GetPlainTextWFormatType(), buffer_type);
      return BoolToPPBool(plain || plainw);
    }
    case PP_FLASH_CLIPBOARD_FORMAT_HTML:
      return BoolToPPBool(clipboard_client_->IsFormatAvailable(
          ui::Clipboard::GetHtmlFormatType(), buffer_type));
    case PP_FLASH_CLIPBOARD_FORMAT_RTF:
      return BoolToPPBool(clipboard_client_->IsFormatAvailable(
          ui::Clipboard::GetRtfFormatType(), buffer_type));
    case PP_FLASH_CLIPBOARD_FORMAT_INVALID:
      break;
  }

  return PP_FALSE;
}

PP_Var PPB_Flash_Impl::ReadClipboardData(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    PP_Flash_Clipboard_Format format) {
  if (!InitClipboard())
    return PP_MakeUndefined();

  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_MakeUndefined();
  }

  if (!IsClipboardFormatAvailable(instance, clipboard_type, format))
    return PP_MakeNull();

  ui::Clipboard::Buffer buffer_type = ConvertClipboardType(clipboard_type);

  switch (format) {
    case PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT: {
      if (clipboard_client_->IsFormatAvailable(
              ui::Clipboard::GetPlainTextWFormatType(), buffer_type)) {
        string16 text;
        clipboard_client_->ReadText(buffer_type, &text);
        if (!text.empty())
          return StringVar::StringToPPVar(UTF16ToUTF8(text));
      }

      if (clipboard_client_->IsFormatAvailable(
              ui::Clipboard::GetPlainTextFormatType(), buffer_type)) {
        std::string text;
        clipboard_client_->ReadAsciiText(buffer_type, &text);
        if (!text.empty())
          return StringVar::StringToPPVar(text);
      }

      return PP_MakeNull();
    }
    case PP_FLASH_CLIPBOARD_FORMAT_HTML: {
      string16 html_stdstr;
      GURL gurl;
      uint32 fragment_start;
      uint32 fragment_end;
      clipboard_client_->ReadHTML(buffer_type,
                                  &html_stdstr,
                                  &gurl,
                                  &fragment_start,
                                  &fragment_end);
      return StringVar::StringToPPVar(UTF16ToUTF8(html_stdstr));
    }
    case PP_FLASH_CLIPBOARD_FORMAT_RTF: {
      std::string result;
      clipboard_client_->ReadRTF(buffer_type, &result);
      return ::ppapi::PpapiGlobals::Get()->GetVarTracker()->
          MakeArrayBufferPPVar(result.size(), result.data());
    }
    case PP_FLASH_CLIPBOARD_FORMAT_INVALID:
      break;
  }

  return PP_MakeUndefined();
}

int32_t PPB_Flash_Impl::WriteClipboardData(
    PP_Instance instance,
    PP_Flash_Clipboard_Type clipboard_type,
    uint32_t data_item_count,
    const PP_Flash_Clipboard_Format formats[],
    const PP_Var data_items[]) {
  if (!InitClipboard())
    return PP_ERROR_FAILED;

  if (clipboard_type != PP_FLASH_CLIPBOARD_TYPE_STANDARD) {
    NOTIMPLEMENTED();
    return PP_ERROR_FAILED;
  }

  if (data_item_count == 0) {
    clipboard_client_->Clear(ConvertClipboardType(clipboard_type));
    return PP_OK;
  }
  ScopedClipboardWriterGlue scw(clipboard_client_.get());
  for (uint32_t i = 0; i < data_item_count; ++i) {
    int32_t res = WriteClipboardDataItem(formats[i], data_items[i], &scw);
    if (res != PP_OK) {
      // Need to clear the objects so nothing is written.
      scw.Reset();
      return res;
    }
  }

  return PP_OK;
}

PP_Bool PPB_Flash_Impl::FlashIsFullscreen(PP_Instance instance) {
  return PP_FromBool(instance_->flash_fullscreen());
}

PP_Bool PPB_Flash_Impl::FlashSetFullscreen(PP_Instance instance,
                                           PP_Bool fullscreen) {
  instance_->FlashSetFullscreen(PP_ToBool(fullscreen), true);
  return PP_TRUE;
}

PP_Bool PPB_Flash_Impl::FlashGetScreenSize(PP_Instance instance,
                                           PP_Size* size) {
  return instance_->GetScreenSize(instance, size);
}

bool PPB_Flash_Impl::InitClipboard() {
  // Initialize the ClipboardClient for writing to the clipboard.
  if (!clipboard_client_.get()) {
    if (!instance_)
      return false;
    PluginDelegate* plugin_delegate = instance_->delegate();
    if (!plugin_delegate)
      return false;
    clipboard_client_.reset(plugin_delegate->CreateClipboardClient());
  }
  return true;
}

int32_t PPB_Flash_Impl::WriteClipboardDataItem(
    const PP_Flash_Clipboard_Format format,
    const PP_Var& data,
    ScopedClipboardWriterGlue* scw) {
  switch (format) {
    case PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT: {
      StringVar* text_string = StringVar::FromPPVar(data);
      if (!text_string)
        return PP_ERROR_BADARGUMENT;

      if (text_string->value().length() > kMaxClipboardWriteSize)
        return PP_ERROR_NOSPACE;

      scw->WriteText(UTF8ToUTF16(text_string->value()));
      return PP_OK;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_HTML: {
      StringVar* text_string = StringVar::FromPPVar(data);
      if (!text_string)
        return PP_ERROR_BADARGUMENT;

      if (text_string->value().length() > kMaxClipboardWriteSize)
        return PP_ERROR_NOSPACE;

      scw->WriteHTML(UTF8ToUTF16(text_string->value()), "");
      return PP_OK;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_RTF: {
      ::ppapi::ArrayBufferVar* rtf_data =
          ::ppapi::ArrayBufferVar::FromPPVar(data);
      if (!rtf_data)
        return PP_ERROR_BADARGUMENT;

      if (rtf_data->ByteLength() > kMaxClipboardWriteSize)
        return PP_ERROR_NOSPACE;

      scw->WriteRTF(std::string(static_cast<char*>(rtf_data->Map()),
                                rtf_data->ByteLength()));
      return PP_OK;
    }
    case PP_FLASH_CLIPBOARD_FORMAT_INVALID:
      break;
  }

  return PP_ERROR_BADARGUMENT;
}

}  // namespace ppapi
}  // namespace webkit
