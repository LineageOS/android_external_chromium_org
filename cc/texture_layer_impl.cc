// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/texture_layer_impl.h"

#include "base/stringprintf.h"
#include "cc/layer_tree_impl.h"
#include "cc/quad_sink.h"
#include "cc/renderer.h"
#include "cc/texture_draw_quad.h"

namespace cc {

TextureLayerImpl::TextureLayerImpl(LayerTreeImpl* treeImpl, int id, bool usesMailbox)
    : LayerImpl(treeImpl, id)
    , m_textureId(0)
    , m_externalTextureResource(0)
    , m_premultipliedAlpha(true)
    , m_flipped(true)
    , m_uvTopLeft(0.f, 0.f)
    , m_uvBottomRight(1.f, 1.f)
    , m_usesMailbox(usesMailbox)
    , m_ownMailbox(false)
{
  m_vertexOpacity[0] = 1.0f;
  m_vertexOpacity[1] = 1.0f;
  m_vertexOpacity[2] = 1.0f;
  m_vertexOpacity[3] = 1.0f;
}

TextureLayerImpl::~TextureLayerImpl()
{
    freeTextureMailbox();
}

void TextureLayerImpl::setTextureMailbox(const TextureMailbox& mailbox)
{
    DCHECK(m_usesMailbox);
    DCHECK(mailbox.IsEmpty() || !mailbox.Equals(m_textureMailbox));
    freeTextureMailbox();
    m_textureMailbox = mailbox;
    m_ownMailbox = true;
}

scoped_ptr<LayerImpl> TextureLayerImpl::createLayerImpl(LayerTreeImpl* treeImpl)
{
    return TextureLayerImpl::create(treeImpl, id(), m_usesMailbox).PassAs<LayerImpl>();
}

void TextureLayerImpl::pushPropertiesTo(LayerImpl* layer)
{
    LayerImpl::pushPropertiesTo(layer);

    TextureLayerImpl* textureLayer = static_cast<TextureLayerImpl*>(layer);
    textureLayer->setFlipped(m_flipped);
    textureLayer->setUVTopLeft(m_uvTopLeft);
    textureLayer->setUVBottomRight(m_uvBottomRight);
    textureLayer->setVertexOpacity(m_vertexOpacity);
    textureLayer->setPremultipliedAlpha(m_premultipliedAlpha);
    if (m_usesMailbox && m_ownMailbox) {
        textureLayer->setTextureMailbox(m_textureMailbox);
        m_ownMailbox = false;
    } else {
        textureLayer->setTextureId(m_textureId);
    }
}


void TextureLayerImpl::willDraw(ResourceProvider* resourceProvider)
{
    if (m_usesMailbox || !m_textureId)
        return;
    DCHECK(!m_externalTextureResource);
    m_externalTextureResource = resourceProvider->createResourceFromExternalTexture(m_textureId);
}

void TextureLayerImpl::appendQuads(QuadSink& quadSink, AppendQuadsData& appendQuadsData)
{
    if (!m_externalTextureResource)
        return;

    SharedQuadState* sharedQuadState = quadSink.useSharedQuadState(createSharedQuadState());
    appendDebugBorderQuad(quadSink, sharedQuadState, appendQuadsData);

    gfx::Rect quadRect(gfx::Point(), contentBounds());
    gfx::Rect opaqueRect(contentsOpaque() ? quadRect : gfx::Rect());
    scoped_ptr<TextureDrawQuad> quad = TextureDrawQuad::Create();
    quad->SetNew(sharedQuadState, quadRect, opaqueRect, m_externalTextureResource, m_premultipliedAlpha, m_uvTopLeft, m_uvBottomRight, m_vertexOpacity, m_flipped);

    // Perform explicit clipping on a quad to avoid setting a scissor later.
    if (sharedQuadState->is_clipped && quad->PerformClipping())
        sharedQuadState->is_clipped = false;
    if (!quad->rect.IsEmpty())
        quadSink.append(quad.PassAs<DrawQuad>(), appendQuadsData);
}

void TextureLayerImpl::didDraw(ResourceProvider* resourceProvider)
{
    if (m_usesMailbox || !m_externalTextureResource)
        return;
    // FIXME: the following assert will not be true when sending resources to a
    // parent compositor. A synchronization scheme (double-buffering or
    // pipelining of updates) for the client will need to exist to solve this.
    DCHECK(!resourceProvider->inUseByConsumer(m_externalTextureResource));
    resourceProvider->deleteResource(m_externalTextureResource);
    m_externalTextureResource = 0;
}

void TextureLayerImpl::dumpLayerProperties(std::string* str, int indent) const
{
    str->append(indentString(indent));
    base::StringAppendF(str, "texture layer texture id: %u premultiplied: %d\n", m_textureId, m_premultipliedAlpha);
    LayerImpl::dumpLayerProperties(str, indent);
}

void TextureLayerImpl::setVertexOpacity(const float vertexOpacity[4]) {
    m_vertexOpacity[0] = vertexOpacity[0];
    m_vertexOpacity[1] = vertexOpacity[1];
    m_vertexOpacity[2] = vertexOpacity[2];
    m_vertexOpacity[3] = vertexOpacity[3];
}

void TextureLayerImpl::didLoseOutputSurface()
{
    m_textureId = 0;
    m_externalTextureResource = 0;
}

const char* TextureLayerImpl::layerTypeAsString() const
{
    return "TextureLayer";
}

bool TextureLayerImpl::canClipSelf() const
{
    return true;
}

void TextureLayerImpl::didBecomeActive()
{
    if (!m_ownMailbox)
        return;
    DCHECK(!m_externalTextureResource);
    ResourceProvider* resourceProvider = layerTreeImpl()->resource_provider();
    if (!m_textureMailbox.IsEmpty())
        m_externalTextureResource = resourceProvider->createResourceFromTextureMailbox(m_textureMailbox);
    m_ownMailbox = false;
}

void TextureLayerImpl::freeTextureMailbox()
{
    if (!m_usesMailbox)
        return;
    if (m_ownMailbox) {
        DCHECK(!m_externalTextureResource);
        m_textureMailbox.RunReleaseCallback(m_textureMailbox.sync_point());
    } else if (m_externalTextureResource) {
        DCHECK(!m_ownMailbox);
        ResourceProvider* resourceProvider = layerTreeImpl()->resource_provider();
        resourceProvider->deleteResource(m_externalTextureResource);
        m_externalTextureResource = 0;
    }
}

}  // namespace cc
