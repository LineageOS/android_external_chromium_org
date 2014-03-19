// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/renderer/test_runner/TestPlugin.h"

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "content/public/renderer/render_thread.h"
#include "content/shell/renderer/test_runner/TestCommon.h"
#include "content/shell/renderer/test_runner/WebTestDelegate.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebCompositorSupport.h"
#include "third_party/WebKit/public/platform/WebGraphicsContext3D.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebTouchPoint.h"
#include "third_party/WebKit/public/web/WebUserGestureIndicator.h"

using namespace blink;
using namespace std;

namespace WebTestRunner {

namespace {

// GLenum values copied from gl2.h.
#define GL_FALSE                  0
#define GL_TRUE                   1
#define GL_ONE                    1
#define GL_TRIANGLES              0x0004
#define GL_ONE_MINUS_SRC_ALPHA    0x0303
#define GL_DEPTH_TEST             0x0B71
#define GL_BLEND                  0x0BE2
#define GL_SCISSOR_TEST           0x0B90
#define GL_TEXTURE_2D             0x0DE1
#define GL_FLOAT                  0x1406
#define GL_RGBA                   0x1908
#define GL_UNSIGNED_BYTE          0x1401
#define GL_TEXTURE_MAG_FILTER     0x2800
#define GL_TEXTURE_MIN_FILTER     0x2801
#define GL_TEXTURE_WRAP_S         0x2802
#define GL_TEXTURE_WRAP_T         0x2803
#define GL_NEAREST                0x2600
#define GL_COLOR_BUFFER_BIT       0x4000
#define GL_CLAMP_TO_EDGE          0x812F
#define GL_ARRAY_BUFFER           0x8892
#define GL_STATIC_DRAW            0x88E4
#define GL_FRAGMENT_SHADER        0x8B30
#define GL_VERTEX_SHADER          0x8B31
#define GL_COMPILE_STATUS         0x8B81
#define GL_LINK_STATUS            0x8B82
#define GL_COLOR_ATTACHMENT0      0x8CE0
#define GL_FRAMEBUFFER_COMPLETE   0x8CD5
#define GL_FRAMEBUFFER            0x8D40

void premultiplyAlpha(const unsigned colorIn[3], float alpha, float colorOut[4])
{
    for (int i = 0; i < 3; ++i)
        colorOut[i] = (colorIn[i] / 255.0f) * alpha;

    colorOut[3] = alpha;
}

const char* pointState(WebTouchPoint::State state)
{
    switch (state) {
    case WebTouchPoint::StateReleased:
        return "Released";
    case WebTouchPoint::StatePressed:
        return "Pressed";
    case WebTouchPoint::StateMoved:
        return "Moved";
    case WebTouchPoint::StateCancelled:
        return "Cancelled";
    default:
        return "Unknown";
    }
}

void printTouchList(WebTestDelegate* delegate, const WebTouchPoint* points, int length)
{
    for (int i = 0; i < length; ++i) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "* %d, %d: %s\n", static_cast<int>(points[i].position.x), static_cast<int>(points[i].position.y), pointState(points[i].state));
        delegate->printMessage(buffer);
    }
}

void printEventDetails(WebTestDelegate* delegate, const WebInputEvent& event)
{
    if (WebInputEvent::isTouchEventType(event.type)) {
        const WebTouchEvent& touch = static_cast<const WebTouchEvent&>(event);
        printTouchList(delegate, touch.touches, touch.touchesLength);
        printTouchList(delegate, touch.changedTouches, touch.changedTouchesLength);
        printTouchList(delegate, touch.targetTouches, touch.targetTouchesLength);
    } else if (WebInputEvent::isMouseEventType(event.type) || event.type == WebInputEvent::MouseWheel) {
        const WebMouseEvent& mouse = static_cast<const WebMouseEvent&>(event);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "* %d, %d\n", mouse.x, mouse.y);
        delegate->printMessage(buffer);
    } else if (WebInputEvent::isGestureEventType(event.type)) {
        const WebGestureEvent& gesture = static_cast<const WebGestureEvent&>(event);
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "* %d, %d\n", gesture.x, gesture.y);
        delegate->printMessage(buffer);
    }
}

WebPluginContainer::TouchEventRequestType parseTouchEventRequestType(const WebString& string)
{
    if (string == WebString::fromUTF8("raw"))
        return WebPluginContainer::TouchEventRequestTypeRaw;
    if (string == WebString::fromUTF8("synthetic"))
        return WebPluginContainer::TouchEventRequestTypeSynthesizedMouse;
    return WebPluginContainer::TouchEventRequestTypeNone;
}

void deferredDelete(void* context)
{
    TestPlugin* plugin = static_cast<TestPlugin*>(context);
    delete plugin;
}

}

TestPlugin::TestPlugin(WebFrame* frame, const WebPluginParams& params, WebTestDelegate* delegate)
    : m_frame(frame)
    , m_delegate(delegate)
    , m_container(0)
    , m_context(0)
    , m_colorTexture(0)
    , m_mailboxChanged(false)
    , m_framebuffer(0)
    , m_touchEventRequest(WebPluginContainer::TouchEventRequestTypeNone)
    , m_reRequestTouchEvents(false)
    , m_printEventDetails(false)
    , m_printUserGestureStatus(false)
    , m_canProcessDrag(false)
    , m_isPersistent(params.mimeType == pluginPersistsMimeType())
    , m_canCreateWithoutRenderer(params.mimeType == canCreateWithoutRendererMimeType())
{
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributePrimitive, ("primitive"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributeBackgroundColor, ("background-color"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributePrimitiveColor, ("primitive-color"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributeOpacity, ("opacity"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributeAcceptsTouch, ("accepts-touch"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributeReRequestTouchEvents, ("re-request-touch"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributePrintEventDetails, ("print-event-details"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributeCanProcessDrag, ("can-process-drag"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kAttributePrintUserGestureStatus, ("print-user-gesture-status"));

    BLINK_ASSERT(params.attributeNames.size() == params.attributeValues.size());
    size_t size = params.attributeNames.size();
    for (size_t i = 0; i < size; ++i) {
        const WebString& attributeName = params.attributeNames[i];
        const WebString& attributeValue = params.attributeValues[i];

        if (attributeName == kAttributePrimitive)
            m_scene.primitive = parsePrimitive(attributeValue);
        else if (attributeName == kAttributeBackgroundColor)
            parseColor(attributeValue, m_scene.backgroundColor);
        else if (attributeName == kAttributePrimitiveColor)
            parseColor(attributeValue, m_scene.primitiveColor);
        else if (attributeName == kAttributeOpacity)
            m_scene.opacity = parseOpacity(attributeValue);
        else if (attributeName == kAttributeAcceptsTouch)
            m_touchEventRequest = parseTouchEventRequestType(attributeValue);
        else if (attributeName == kAttributeReRequestTouchEvents)
            m_reRequestTouchEvents = parseBoolean(attributeValue);
        else if (attributeName == kAttributePrintEventDetails)
            m_printEventDetails = parseBoolean(attributeValue);
        else if (attributeName == kAttributeCanProcessDrag)
            m_canProcessDrag = parseBoolean(attributeValue);
        else if (attributeName == kAttributePrintUserGestureStatus)
            m_printUserGestureStatus = parseBoolean(attributeValue);
    }
    if (m_canCreateWithoutRenderer)
        m_delegate->printMessage(std::string("TestPlugin: canCreateWithoutRenderer\n"));
}

TestPlugin::~TestPlugin()
{
}

bool TestPlugin::initialize(WebPluginContainer* container)
{
    WebGraphicsContext3D::Attributes attrs;
    m_context = Platform::current()->createOffscreenGraphicsContext3D(attrs);
    if (m_context && !m_context->makeContextCurrent()) {
        delete m_context;
        m_context = 0;
    }

    if (!initScene())
        return false;

    m_layer = cc::TextureLayer::CreateForMailbox(this);
    m_webLayer = make_scoped_ptr(new webkit::WebLayerImpl(m_layer));
    m_container = container;
    m_container->setWebLayer(m_webLayer.get());
    if (m_reRequestTouchEvents) {
        m_container->requestTouchEventType(WebPluginContainer::TouchEventRequestTypeSynthesizedMouse);
        m_container->requestTouchEventType(WebPluginContainer::TouchEventRequestTypeRaw);
    }
    m_container->requestTouchEventType(m_touchEventRequest);
    m_container->setWantsWheelEvents(true);
    return true;
}

void TestPlugin::destroy()
{
    if (m_layer.get()) {
        m_layer->WillModifyTexture();
        m_layer->SetTextureMailbox(cc::TextureMailbox(),
                                   scoped_ptr<cc::SingleReleaseCallback>());
    }
    if (m_container)
        m_container->setWebLayer(0);
    m_webLayer.reset();
    m_layer = NULL;
    destroyScene();

    delete m_context;
    m_context = 0;

    m_container = 0;
    m_frame = 0;

    Platform::current()->callOnMainThread(deferredDelete, this);
}

NPObject* TestPlugin::scriptableObject()
{
    return 0;
}

bool TestPlugin::canProcessDrag() const
{
    return m_canProcessDrag;
}

void TestPlugin::updateGeometry(const WebRect& frameRect, const WebRect& clipRect, const WebVector<WebRect>& cutOutsRects, bool isVisible)
{
    if (clipRect == m_rect)
        return;
    m_rect = clipRect;

    if (m_rect.isEmpty()) {
        m_textureMailbox = cc::TextureMailbox();
    } else if (m_context) {
        m_context->viewport(0, 0, m_rect.width, m_rect.height);

        m_context->bindTexture(GL_TEXTURE_2D, m_colorTexture);
        m_context->texParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_context->texParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_context->texParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_context->texParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        m_context->texImage2D(GL_TEXTURE_2D,
                              0,
                              GL_RGBA,
                              m_rect.width,
                              m_rect.height,
                              0,
                              GL_RGBA,
                              GL_UNSIGNED_BYTE,
                              0);
        m_context->bindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        m_context->framebufferTexture2D(GL_FRAMEBUFFER,
                                        GL_COLOR_ATTACHMENT0,
                                        GL_TEXTURE_2D,
                                        m_colorTexture,
                                        0);

        drawSceneGL();

        gpu::Mailbox mailbox;
        m_context->genMailboxCHROMIUM(mailbox.name);
        m_context->produceTextureCHROMIUM(GL_TEXTURE_2D, mailbox.name);
        m_context->flush();
        uint32 syncPoint = m_context->insertSyncPoint();
        m_textureMailbox = cc::TextureMailbox(mailbox, GL_TEXTURE_2D, syncPoint);
    } else {
        size_t bytes = 4 * m_rect.width * m_rect.height;
        scoped_ptr<base::SharedMemory> bitmap =
                content::RenderThread::Get()->HostAllocateSharedMemoryBuffer(bytes);
        if (!bitmap->Map(bytes)) {
            m_textureMailbox = cc::TextureMailbox();
        } else {
            drawSceneSoftware(bitmap->memory(), bytes);
            m_textureMailbox = cc::TextureMailbox(
                bitmap.get(), gfx::Size(m_rect.width, m_rect.height));
            m_sharedBitmap = bitmap.Pass();
        }
    }

    m_mailboxChanged = true;
    m_layer->SetNeedsDisplay();
}

bool TestPlugin::acceptsInputEvents()
{
    return true;
}

bool TestPlugin::isPlaceholder()
{
    return false;
}

unsigned TestPlugin::PrepareTexture() {
    NOTREACHED();
    return 0;
}

static void ignoreReleaseCallback(uint32 sync_point, bool lost) {}

static void releaseSharedMemory(scoped_ptr<base::SharedMemory> bitmap,
                                uint32 sync_point,
                                bool lost) {}

bool TestPlugin::PrepareTextureMailbox(
    cc::TextureMailbox* mailbox,
    scoped_ptr<cc::SingleReleaseCallback>* releaseCallback,
    bool useSharedMemory) {
    if (!m_mailboxChanged)
        return false;
    *mailbox = m_textureMailbox;
    if (m_textureMailbox.IsTexture()) {
      *releaseCallback =
          cc::SingleReleaseCallback::Create(base::Bind(&ignoreReleaseCallback));
    } else {
      *releaseCallback = cc::SingleReleaseCallback::Create(
          base::Bind(&releaseSharedMemory, base::Passed(&m_sharedBitmap)));
    }
    m_mailboxChanged = false;
    return true;
}

TestPlugin::Primitive TestPlugin::parsePrimitive(const WebString& string)
{
    const CR_DEFINE_STATIC_LOCAL(WebString, kPrimitiveNone, ("none"));
    const CR_DEFINE_STATIC_LOCAL(WebString, kPrimitiveTriangle, ("triangle"));

    Primitive primitive = PrimitiveNone;
    if (string == kPrimitiveNone)
        primitive = PrimitiveNone;
    else if (string == kPrimitiveTriangle)
        primitive = PrimitiveTriangle;
    else
        BLINK_ASSERT_NOT_REACHED();
    return primitive;
}

// FIXME: This method should already exist. Use it.
// For now just parse primary colors.
void TestPlugin::parseColor(const WebString& string, unsigned color[3])
{
    color[0] = color[1] = color[2] = 0;
    if (string == "black")
        return;

    if (string == "red")
        color[0] = 255;
    else if (string == "green")
        color[1] = 255;
    else if (string == "blue")
        color[2] = 255;
    else
        BLINK_ASSERT_NOT_REACHED();
}

float TestPlugin::parseOpacity(const WebString& string)
{
    return static_cast<float>(atof(string.utf8().data()));
}

bool TestPlugin::parseBoolean(const WebString& string)
{
    const CR_DEFINE_STATIC_LOCAL(WebString, kPrimitiveTrue, ("true"));
    return string == kPrimitiveTrue;
}

bool TestPlugin::initScene()
{
    if (!m_context)
        return true;

    float color[4];
    premultiplyAlpha(m_scene.backgroundColor, m_scene.opacity, color);

    m_colorTexture = m_context->createTexture();
    m_framebuffer = m_context->createFramebuffer();

    m_context->viewport(0, 0, m_rect.width, m_rect.height);
    m_context->disable(GL_DEPTH_TEST);
    m_context->disable(GL_SCISSOR_TEST);

    m_context->clearColor(color[0], color[1], color[2], color[3]);

    m_context->enable(GL_BLEND);
    m_context->blendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    return m_scene.primitive != PrimitiveNone ? initProgram() && initPrimitive() : true;
}

void TestPlugin::drawSceneGL() {
    m_context->viewport(0, 0, m_rect.width, m_rect.height);
    m_context->clear(GL_COLOR_BUFFER_BIT);

    if (m_scene.primitive != PrimitiveNone)
        drawPrimitive();
}

void TestPlugin::drawSceneSoftware(void* memory, size_t bytes) {
    DCHECK_EQ(bytes, m_rect.width * m_rect.height * 4u);

    SkColor backgroundColor =
            SkColorSetARGB(static_cast<uint8>(m_scene.opacity * 255),
                           m_scene.backgroundColor[0],
                           m_scene.backgroundColor[1],
                           m_scene.backgroundColor[2]);

    SkBitmap bitmap;
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, m_rect.width, m_rect.height);
    bitmap.setPixels(memory);
    SkCanvas canvas(bitmap);
    canvas.clear(backgroundColor);

    if (m_scene.primitive != PrimitiveNone) {
        DCHECK_EQ(PrimitiveTriangle, m_scene.primitive);
        SkColor foregroundColor =
                SkColorSetARGB(static_cast<uint8>(m_scene.opacity * 255),
                               m_scene.primitiveColor[0],
                               m_scene.primitiveColor[1],
                               m_scene.primitiveColor[2]);
        SkPath trianglePath;
        trianglePath.moveTo(0.5f * m_rect.width, 0.9f * m_rect.height);
        trianglePath.lineTo(0.1f * m_rect.width, 0.1f * m_rect.height);
        trianglePath.lineTo(0.9f * m_rect.width, 0.1f * m_rect.height);
        SkPaint paint;
        paint.setColor(foregroundColor);
        paint.setStyle(SkPaint::kFill_Style);
        canvas.drawPath(trianglePath, paint);
    }
}

void TestPlugin::destroyScene()
{
    if (m_scene.program) {
        m_context->deleteProgram(m_scene.program);
        m_scene.program = 0;
    }
    if (m_scene.vbo) {
        m_context->deleteBuffer(m_scene.vbo);
        m_scene.vbo = 0;
    }

    if (m_framebuffer) {
        m_context->deleteFramebuffer(m_framebuffer);
        m_framebuffer = 0;
    }

    if (m_colorTexture) {
        m_context->deleteTexture(m_colorTexture);
        m_colorTexture = 0;
    }
}

bool TestPlugin::initProgram()
{
    const string vertexSource(
        "attribute vec4 position;  \n"
        "void main() {             \n"
        "  gl_Position = position; \n"
        "}                         \n"
    );

    const string fragmentSource(
        "precision mediump float; \n"
        "uniform vec4 color;      \n"
        "void main() {            \n"
        "  gl_FragColor = color;  \n"
        "}                        \n"
    );

    m_scene.program = loadProgram(vertexSource, fragmentSource);
    if (!m_scene.program)
        return false;

    m_scene.colorLocation = m_context->getUniformLocation(m_scene.program, "color");
    m_scene.positionLocation = m_context->getAttribLocation(m_scene.program, "position");
    return true;
}

bool TestPlugin::initPrimitive()
{
    BLINK_ASSERT(m_scene.primitive == PrimitiveTriangle);

    m_scene.vbo = m_context->createBuffer();
    if (!m_scene.vbo)
        return false;

    const float vertices[] = {
        0.0f,  0.8f, 0.0f,
        -0.8f, -0.8f, 0.0f,
        0.8f, -0.8f, 0.0f };
    m_context->bindBuffer(GL_ARRAY_BUFFER, m_scene.vbo);
    m_context->bufferData(GL_ARRAY_BUFFER, sizeof(vertices), 0, GL_STATIC_DRAW);
    m_context->bufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    return true;
}

void TestPlugin::drawPrimitive()
{
    BLINK_ASSERT(m_scene.primitive == PrimitiveTriangle);
    BLINK_ASSERT(m_scene.vbo);
    BLINK_ASSERT(m_scene.program);

    m_context->useProgram(m_scene.program);

    // Bind primitive color.
    float color[4];
    premultiplyAlpha(m_scene.primitiveColor, m_scene.opacity, color);
    m_context->uniform4f(m_scene.colorLocation, color[0], color[1], color[2], color[3]);

    // Bind primitive vertices.
    m_context->bindBuffer(GL_ARRAY_BUFFER, m_scene.vbo);
    m_context->enableVertexAttribArray(m_scene.positionLocation);
    m_context->vertexAttribPointer(m_scene.positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    m_context->drawArrays(GL_TRIANGLES, 0, 3);
}

unsigned TestPlugin::loadShader(unsigned type, const string& source)
{
    unsigned shader = m_context->createShader(type);
    if (shader) {
        m_context->shaderSource(shader, source.data());
        m_context->compileShader(shader);

        int compiled = 0;
        m_context->getShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            m_context->deleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

unsigned TestPlugin::loadProgram(const string& vertexSource, const string& fragmentSource)
{
    unsigned vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    unsigned fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    unsigned program = m_context->createProgram();
    if (vertexShader && fragmentShader && program) {
        m_context->attachShader(program, vertexShader);
        m_context->attachShader(program, fragmentShader);
        m_context->linkProgram(program);

        int linked = 0;
        m_context->getProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            m_context->deleteProgram(program);
            program = 0;
        }
    }
    if (vertexShader)
        m_context->deleteShader(vertexShader);
    if (fragmentShader)
        m_context->deleteShader(fragmentShader);

    return program;
}

bool TestPlugin::handleInputEvent(const WebInputEvent& event, WebCursorInfo& info)
{
    const char* eventName = 0;
    switch (event.type) {
    case WebInputEvent::Undefined:           eventName = "unknown"; break;

    case WebInputEvent::MouseDown:           eventName = "MouseDown"; break;
    case WebInputEvent::MouseUp:             eventName = "MouseUp"; break;
    case WebInputEvent::MouseMove:           eventName = "MouseMove"; break;
    case WebInputEvent::MouseEnter:          eventName = "MouseEnter"; break;
    case WebInputEvent::MouseLeave:          eventName = "MouseLeave"; break;
    case WebInputEvent::ContextMenu:         eventName = "ContextMenu"; break;

    case WebInputEvent::MouseWheel:          eventName = "MouseWheel"; break;

    case WebInputEvent::RawKeyDown:          eventName = "RawKeyDown"; break;
    case WebInputEvent::KeyDown:             eventName = "KeyDown"; break;
    case WebInputEvent::KeyUp:               eventName = "KeyUp"; break;
    case WebInputEvent::Char:                eventName = "Char"; break;

    case WebInputEvent::GestureScrollBegin:  eventName = "GestureScrollBegin"; break;
    case WebInputEvent::GestureScrollEnd:    eventName = "GestureScrollEnd"; break;
    case WebInputEvent::GestureScrollUpdateWithoutPropagation:
    case WebInputEvent::GestureScrollUpdate: eventName = "GestureScrollUpdate"; break;
    case WebInputEvent::GestureFlingStart:   eventName = "GestureFlingStart"; break;
    case WebInputEvent::GestureFlingCancel:  eventName = "GestureFlingCancel"; break;
    case WebInputEvent::GestureTap:          eventName = "GestureTap"; break;
    case WebInputEvent::GestureTapUnconfirmed:
                                             eventName = "GestureTapUnconfirmed"; break;
    case WebInputEvent::GestureTapDown:      eventName = "GestureTapDown"; break;
    case WebInputEvent::GestureShowPress:    eventName = "GestureShowPress"; break;
    case WebInputEvent::GestureTapCancel:    eventName = "GestureTapCancel"; break;
    case WebInputEvent::GestureDoubleTap:    eventName = "GestureDoubleTap"; break;
    case WebInputEvent::GestureTwoFingerTap: eventName = "GestureTwoFingerTap"; break;
    case WebInputEvent::GestureLongPress:    eventName = "GestureLongPress"; break;
    case WebInputEvent::GestureLongTap:      eventName = "GestureLongTap"; break;
    case WebInputEvent::GesturePinchBegin:   eventName = "GesturePinchBegin"; break;
    case WebInputEvent::GesturePinchEnd:     eventName = "GesturePinchEnd"; break;
    case WebInputEvent::GesturePinchUpdate:  eventName = "GesturePinchUpdate"; break;

    case WebInputEvent::TouchStart:          eventName = "TouchStart"; break;
    case WebInputEvent::TouchMove:           eventName = "TouchMove"; break;
    case WebInputEvent::TouchEnd:            eventName = "TouchEnd"; break;
    case WebInputEvent::TouchCancel:         eventName = "TouchCancel"; break;
    }

    m_delegate->printMessage(std::string("Plugin received event: ") + (eventName ? eventName : "unknown") + "\n");
    if (m_printEventDetails)
        printEventDetails(m_delegate, event);
    if (m_printUserGestureStatus)
        m_delegate->printMessage(std::string("* ") + (WebUserGestureIndicator::isProcessingUserGesture() ? "" : "not ") + "handling user gesture\n");
    if (m_isPersistent)
        m_delegate->printMessage(std::string("TestPlugin: isPersistent\n"));
    return false;
}

bool TestPlugin::handleDragStatusUpdate(WebDragStatus dragStatus, const WebDragData&, WebDragOperationsMask, const WebPoint& position, const WebPoint& screenPosition)
{
    const char* dragStatusName = 0;
    switch (dragStatus) {
    case WebDragStatusEnter:
        dragStatusName = "DragEnter";
        break;
    case WebDragStatusOver:
        dragStatusName = "DragOver";
        break;
    case WebDragStatusLeave:
        dragStatusName = "DragLeave";
        break;
    case WebDragStatusDrop:
        dragStatusName = "DragDrop";
        break;
    case WebDragStatusUnknown:
        BLINK_ASSERT_NOT_REACHED();
    }
    m_delegate->printMessage(std::string("Plugin received event: ") + dragStatusName + "\n");
    return false;
}

TestPlugin* TestPlugin::create(WebFrame* frame, const WebPluginParams& params, WebTestDelegate* delegate)
{
    return new TestPlugin(frame, params, delegate);
}

const WebString& TestPlugin::mimeType()
{
    const CR_DEFINE_STATIC_LOCAL(WebString, kMimeType, ("application/x-webkit-test-webplugin"));
    return kMimeType;
}

const WebString& TestPlugin::canCreateWithoutRendererMimeType()
{
    const CR_DEFINE_STATIC_LOCAL(WebString, kCanCreateWithoutRendererMimeType, ("application/x-webkit-test-webplugin-can-create-without-renderer"));
    return kCanCreateWithoutRendererMimeType;
}

const WebString& TestPlugin::pluginPersistsMimeType()
{
    const CR_DEFINE_STATIC_LOCAL(WebString, kPluginPersistsMimeType, ("application/x-webkit-test-webplugin-persistent"));
    return kPluginPersistsMimeType;
}

bool TestPlugin::isSupportedMimeType(const WebString& mimeType)
{
    return mimeType == TestPlugin::mimeType()
           || mimeType == pluginPersistsMimeType()
           || mimeType == canCreateWithoutRendererMimeType();
}

}
