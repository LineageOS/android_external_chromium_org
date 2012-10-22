// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_LAYER_TREE_TEST_COMMON_H_
#define CC_TEST_LAYER_TREE_TEST_COMMON_H_

#include "CCLayerTreeHost.h"
#include "base/hash_tables.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/scoped_thread_proxy.h"
#include "cc/test/compositor_fake_web_graphics_context_3d.h"
#include "testing/gtest/include/gtest/gtest.h"
#include <public/WebAnimationDelegate.h>
#include <public/WebThread.h>

namespace cc {
class CCLayerImpl;
class CCLayerTreeHost;
class CCLayerTreeHostClient;
class CCLayerTreeHostImpl;
}

namespace WebKitTests {

// Used by test stubs to notify the test when something interesting happens.
class TestHooks : public WebKit::WebAnimationDelegate {
public:
    virtual void beginCommitOnCCThread(cc::CCLayerTreeHostImpl*) { }
    virtual void commitCompleteOnCCThread(cc::CCLayerTreeHostImpl*) { }
    virtual bool prepareToDrawOnCCThread(cc::CCLayerTreeHostImpl*);
    virtual void drawLayersOnCCThread(cc::CCLayerTreeHostImpl*) { }
    virtual void animateLayers(cc::CCLayerTreeHostImpl*, double monotonicTime) { }
    virtual void willAnimateLayers(cc::CCLayerTreeHostImpl*, double monotonicTime) { }
    virtual void applyScrollAndScale(const cc::IntSize&, float) { }
    virtual void animate(double monotonicTime) { }
    virtual void layout() { }
    virtual void didRecreateOutputSurface(bool succeeded) { }
    virtual void didAddAnimation() { }
    virtual void didCommit() { }
    virtual void didCommitAndDrawFrame() { }
    virtual void scheduleComposite() { }

    // Implementation of WebAnimationDelegate
    virtual void notifyAnimationStarted(double time) OVERRIDE { }
    virtual void notifyAnimationFinished(double time) OVERRIDE { }

    virtual scoped_ptr<WebKit::WebCompositorOutputSurface> createOutputSurface();
};

class TimeoutTask;
class BeginTask;

class MockCCLayerTreeHostClient : public cc::CCLayerTreeHostClient {
};

// The CCThreadedTests runs with the main loop running. It instantiates a single MockLayerTreeHost and associated
// MockLayerTreeHostImpl/MockLayerTreeHostClient.
//
// beginTest() is called once the main message loop is running and the layer tree host is initialized.
//
// Key stages of the drawing loop, e.g. drawing or commiting, redirect to CCThreadedTest methods of similar names.
// To track the commit process, override these functions.
//
// The test continues until someone calls endTest. endTest can be called on any thread, but be aware that
// ending the test is an asynchronous process.
class CCThreadedTest : public testing::Test, public TestHooks {
public:
    virtual ~CCThreadedTest();

    virtual void afterTest() = 0;
    virtual void beginTest() = 0;

    void endTest();
    void endTestAfterDelay(int delayMilliseconds);

    void postSetNeedsAnimateToMainThread();
    void postAddAnimationToMainThread();
    void postAddInstantAnimationToMainThread();
    void postSetNeedsCommitToMainThread();
    void postAcquireLayerTextures();
    void postSetNeedsRedrawToMainThread();
    void postSetNeedsAnimateAndCommitToMainThread();
    void postSetVisibleToMainThread(bool visible);
    void postDidAddAnimationToMainThread();

    void doBeginTest();
    void timeout();

    void clearTimeout() { m_timeoutTask = 0; }

    cc::CCLayerTreeHost* layerTreeHost() { return m_layerTreeHost.get(); }

protected:
    CCThreadedTest();

    virtual void initializeSettings(cc::CCLayerTreeSettings&) { }

    virtual void scheduleComposite() OVERRIDE;

    void realEndTest();

    void dispatchSetNeedsAnimate();
    void dispatchAddInstantAnimation();
    void dispatchAddAnimation();
    void dispatchSetNeedsAnimateAndCommit();
    void dispatchSetNeedsCommit();
    void dispatchAcquireLayerTextures();
    void dispatchSetNeedsRedraw();
    void dispatchSetVisible(bool);
    void dispatchComposite();
    void dispatchDidAddAnimation();

    virtual void runTest(bool threaded);
    WebKit::WebThread* webThread() const { return m_webThread.get(); }

    cc::CCLayerTreeSettings m_settings;
    scoped_ptr<MockCCLayerTreeHostClient> m_client;
    scoped_ptr<cc::CCLayerTreeHost> m_layerTreeHost;

protected:
    RefPtr<cc::CCScopedThreadProxy> m_mainThreadProxy;

private:
    bool m_beginning;
    bool m_endWhenBeginReturns;
    bool m_timedOut;
    bool m_finished;
    bool m_scheduled;
    bool m_started;

    scoped_ptr<WebKit::WebThread> m_webThread;
    TimeoutTask* m_timeoutTask;
    BeginTask* m_beginTask;
};

class CCThreadedTestThreadOnly : public CCThreadedTest {
public:
    void runTestThreaded()
    {
        CCThreadedTest::runTest(true);
    }
};

// Adapts CCLayerTreeHostImpl for test. Runs real code, then invokes test hooks.
class MockLayerTreeHostImpl : public cc::CCLayerTreeHostImpl {
public:
    static scoped_ptr<MockLayerTreeHostImpl> create(TestHooks*, const cc::CCLayerTreeSettings&, cc::CCLayerTreeHostImplClient*);

    virtual void beginCommit() OVERRIDE;
    virtual void commitComplete() OVERRIDE;
    virtual bool prepareToDraw(FrameData&) OVERRIDE;
    virtual void drawLayers(const FrameData&) OVERRIDE;

    // Make these public.
    typedef std::vector<cc::CCLayerImpl*> CCLayerList;
    using CCLayerTreeHostImpl::calculateRenderSurfaceLayerList;

protected:
    virtual void animateLayers(double monotonicTime, double wallClockTime) OVERRIDE;
    virtual base::TimeDelta lowFrequencyAnimationInterval() const OVERRIDE;

private:
    MockLayerTreeHostImpl(TestHooks*, const cc::CCLayerTreeSettings&, cc::CCLayerTreeHostImplClient*);

    TestHooks* m_testHooks;
};

class CompositorFakeWebGraphicsContext3DWithTextureTracking : public WebKit::CompositorFakeWebGraphicsContext3D {
public:
    static scoped_ptr<CompositorFakeWebGraphicsContext3DWithTextureTracking> create(Attributes);
    virtual ~CompositorFakeWebGraphicsContext3DWithTextureTracking();

    virtual WebKit::WebGLId createTexture();

    virtual void deleteTexture(WebKit::WebGLId texture);

    virtual void bindTexture(WebKit::WGC3Denum target, WebKit::WebGLId texture);

    int numTextures() const;
    int texture(int texture) const;
    void resetTextures();

    int numUsedTextures() const;
    bool usedTexture(int texture) const;
    void resetUsedTextures();

private:
    explicit CompositorFakeWebGraphicsContext3DWithTextureTracking(Attributes attrs);

    Vector<WebKit::WebGLId> m_textures;
    base::hash_set<WebKit::WebGLId> m_usedTextures;
};

} // namespace WebKitTests

#define SINGLE_AND_MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME) \
    TEST_F(TEST_FIXTURE_NAME, runSingleThread)            \
    {                                                     \
        runTest(false);                                   \
    }                                                     \
    TEST_F(TEST_FIXTURE_NAME, runMultiThread)             \
    {                                                     \
        runTest(true);                                    \
    }

#endif // CC_TEST_LAYER_TREE_TEST_COMMON_H_
