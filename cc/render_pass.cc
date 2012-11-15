// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/render_pass.h"

#include "cc/layer_impl.h"
#include "cc/math_util.h"
#include "cc/occlusion_tracker.h"
#include "cc/quad_culler.h"
#include "cc/shared_quad_state.h"
#include "cc/solid_color_draw_quad.h"
#include "third_party/skia/include/core/SkImageFilter.h"

using WebKit::WebTransformationMatrix;

namespace cc {

scoped_ptr<RenderPass> RenderPass::create(Id id, gfx::Rect outputRect, const WebKit::WebTransformationMatrix& transformToRootTarget)
{
    return make_scoped_ptr(new RenderPass(id, outputRect, transformToRootTarget));
}

RenderPass::RenderPass(Id id, gfx::Rect outputRect, const WebKit::WebTransformationMatrix& transformToRootTarget)
    : m_id(id)
    , m_transformToRootTarget(transformToRootTarget)
    , m_outputRect(outputRect)
    , m_hasTransparentBackground(true)
    , m_hasOcclusionFromOutsideTargetSurface(false)
    , m_filter(0)
{
    DCHECK(id.layerId > 0);
    DCHECK(id.index >= 0);
}

RenderPass::~RenderPass()
{
    SkSafeUnref(m_filter);
}

scoped_ptr<RenderPass> RenderPass::copy(Id newId) const
{
    DCHECK(newId != m_id);

    scoped_ptr<RenderPass> copyPass(create(newId, m_outputRect, m_transformToRootTarget));
    copyPass->setDamageRect(m_damageRect);
    copyPass->setHasTransparentBackground(m_hasTransparentBackground);
    copyPass->setHasOcclusionFromOutsideTargetSurface(m_hasOcclusionFromOutsideTargetSurface);
    copyPass->setFilters(m_filters);
    copyPass->setBackgroundFilters(m_backgroundFilters);
    copyPass->setFilter(m_filter);
    return copyPass.Pass();
}

void RenderPass::appendQuadsForLayer(LayerImpl* layer, OcclusionTrackerImpl* occlusionTracker, AppendQuadsData& appendQuadsData)
{
    bool forSurface = false;
    QuadCuller quadCuller(m_quadList, m_sharedQuadStateList, layer, occlusionTracker, layer->showDebugBorders(), forSurface);

    layer->appendQuads(quadCuller, appendQuadsData);
}

void RenderPass::appendQuadsForRenderSurfaceLayer(LayerImpl* layer, const RenderPass* contributingRenderPass, OcclusionTrackerImpl* occlusionTracker, AppendQuadsData& appendQuadsData)
{
    bool forSurface = true;
    QuadCuller quadCuller(m_quadList, m_sharedQuadStateList, layer, occlusionTracker, layer->showDebugBorders(), forSurface);

    bool isReplica = false;
    layer->renderSurface()->appendQuads(quadCuller, appendQuadsData, isReplica, contributingRenderPass->id());

    // Add replica after the surface so that it appears below the surface.
    if (layer->hasReplica()) {
        isReplica = true;
        layer->renderSurface()->appendQuads(quadCuller, appendQuadsData, isReplica, contributingRenderPass->id());
    }
}

void RenderPass::appendQuadsToFillScreen(LayerImpl* rootLayer, SkColor screenBackgroundColor, const OcclusionTrackerImpl& occlusionTracker)
{
    if (!rootLayer || !screenBackgroundColor)
        return;

    Region fillRegion = occlusionTracker.computeVisibleRegionInScreen();
    if (fillRegion.IsEmpty())
        return;

    bool forSurface = false;
    QuadCuller quadCuller(m_quadList, m_sharedQuadStateList, rootLayer, &occlusionTracker, rootLayer->showDebugBorders(), forSurface);

    // Manually create the quad state for the gutter quads, as the root layer
    // doesn't have any bounds and so can't generate this itself.
    // FIXME: Make the gutter quads generated by the solid color layer (make it smarter about generating quads to fill unoccluded areas).
    gfx::Rect rootTargetRect = rootLayer->renderSurface()->contentRect();
    float opacity = 1;
    bool opaque = true;
    SharedQuadState* sharedQuadState = quadCuller.useSharedQuadState(SharedQuadState::create(rootLayer->drawTransform(), rootTargetRect, rootTargetRect, opacity, opaque));
    DCHECK(rootLayer->screenSpaceTransform().isInvertible());
    WebTransformationMatrix transformToLayerSpace = rootLayer->screenSpaceTransform().inverse();
    for (Region::Iterator fillRects(fillRegion); fillRects.has_rect(); fillRects.next()) {
        // The root layer transform is composed of translations and scales only, no perspective, so mapping is sufficient.
        gfx::Rect layerRect = MathUtil::mapClippedRect(transformToLayerSpace, fillRects.rect());
        // Skip the quad culler and just append the quads directly to avoid occlusion checks.
        m_quadList.append(SolidColorDrawQuad::create(sharedQuadState, layerRect, screenBackgroundColor).PassAs<DrawQuad>());
    }
}

void RenderPass::setFilter(SkImageFilter* filter) {
    SkRefCnt_SafeAssign(m_filter, filter);
}

}  // namespace cc
