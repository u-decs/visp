/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2024 by Inria. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact Inria about acquiring a ViSP Professional
 * Edition License.
 *
 * See https://visp.inria.fr for more information.
 *
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 *
 * If you have questions regarding the use of this file, please contact
 * Inria at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <visp3/ar/vpPanda3DRendererSet.h>

#if defined(VISP_HAVE_PANDA3D)

#include <visp3/ar/vpPanda3DFrameworkManager.h>


BEGIN_VISP_NAMESPACE
vpPanda3DRendererSet::vpPanda3DRendererSet() : vpPanda3DBaseRenderer("set")
{

}

vpPanda3DRendererSet::vpPanda3DRendererSet(const vpPanda3DRenderParameters &renderParameters) : vpPanda3DBaseRenderer("set")
{
  m_renderParameters = renderParameters;
}

void vpPanda3DRendererSet::initFramework()
{

  vpPanda3DFrameworkManager &frameworkManager = vpPanda3DFrameworkManager::getInstance();
  PandaFramework &framework = frameworkManager.getFramework();
  frameworkManager.initFramework();
  m_isWindowOwner = true;

  if (m_renderParameters.getImageHeight() == 0 || m_renderParameters.getImageWidth() == 0) {
    throw vpException(vpException::badValue,
    "Panda3D renderer: Cannot create a window with 0 height or width.");
  }

  WindowProperties winProps;
  winProps.set_size(LVecBase2i(m_renderParameters.getImageWidth(), m_renderParameters.getImageHeight()));
  int flags = GraphicsPipe::BF_refuse_window;
  m_window = framework.open_window(winProps, flags);
  if (m_window == nullptr) {
    winProps.set_minimized(true);
    m_window = framework.open_window(winProps, 0);
  }
  if (m_window == nullptr) {
    throw vpException(vpException::fatalError, "Could not open Panda3D window (hidden or visible)");
  }

  m_window->set_background_type(WindowFramework::BackgroundType::BT_black);
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    renderer->initFromParent(*this);
  }
}

void vpPanda3DRendererSet::initFromParent(PointerTo<WindowFramework> window)
{
  vpPanda3DBaseRenderer::initFromParent(window);
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    renderer->initFromParent(m_window);
  }
}

void vpPanda3DRendererSet::initFromParent(const vpPanda3DBaseRenderer &renderer)
{
  vpPanda3DBaseRenderer::initFromParent(renderer);
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    renderer->initFromParent(*this);
  }
}

void vpPanda3DRendererSet::setCameraPose(const vpHomogeneousMatrix &wTc)
{
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    if (renderer->isRendering3DScene()) {
      renderer->setCameraPose(wTc);
    }
  }
}

vpHomogeneousMatrix vpPanda3DRendererSet::getCameraPose()
{
  if (m_subRenderers.size() == 0) {
    throw vpException(vpException::fatalError, "cannot get the pose of an object if no subrenderer is present");
  }
  if (!m_subRenderers[0]->isRendering3DScene()) {
    throw vpException(vpException::fatalError, "This renderer set only contains a non-3D renderer.");
  }
  return m_subRenderers[0]->getCameraPose();
}

void vpPanda3DRendererSet::setNodePose(const std::string &name, const vpHomogeneousMatrix &wTo)
{
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    if (renderer->isRendering3DScene()) {
      renderer->setNodePose(name, wTo);
    }
  }
}

void vpPanda3DRendererSet::setNodePose(NodePath &, const vpHomogeneousMatrix &)
{
  throw vpException(vpException::badValue, "NodePath setNodePose is not supported in renderer set, prefer the string version");
}

vpHomogeneousMatrix vpPanda3DRendererSet::getNodePose(const std::string &name)
{
  if (m_subRenderers.size() == 0) {
    throw vpException(vpException::fatalError, "cannot get the pose of an object if no subrenderer is present");
  }
  if (!m_subRenderers[0]->isRendering3DScene()) {
    throw vpException(vpException::fatalError, "This renderer set only contains a non-3D renderer.");
  }
  return m_subRenderers[0]->getNodePose(name);
}

vpHomogeneousMatrix vpPanda3DRendererSet::getNodePose(NodePath &)
{
  throw vpException(vpException::badValue, "NodePath getNodePose is not supported in renderer set, prefer the string version");
}

void vpPanda3DRendererSet::addNodeToScene(const NodePath &object)
{
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    if (renderer->isRendering3DScene()) {
      renderer->addNodeToScene(object);
    }
  }
}

void vpPanda3DRendererSet::setRenderParameters(const vpPanda3DRenderParameters &params)
{
  m_renderParameters = params;
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    renderer->setRenderParameters(m_renderParameters);
  }
}

void vpPanda3DRendererSet::addLight(const vpPanda3DLight &light)
{
  for (std::shared_ptr<vpPanda3DBaseRenderer> &renderer: m_subRenderers) {
    vpPanda3DLightable *lightable = dynamic_cast<vpPanda3DLightable *>(renderer.get());
    if (lightable != nullptr) {
      lightable->addLight(light);
    }
  }
}

void vpPanda3DRendererSet::addSubRenderer(std::shared_ptr<vpPanda3DBaseRenderer> renderer)
{
  for (std::shared_ptr<vpPanda3DBaseRenderer> &otherRenderer: m_subRenderers) {
    if (renderer->getName() == otherRenderer->getName()) {
      throw vpException(vpException::badValue, "Cannot have two subrenderers with the same name");
    }
  }
  std::vector<std::shared_ptr<vpPanda3DBaseRenderer>>::const_iterator it = m_subRenderers.begin();
  while (it != m_subRenderers.end()) {
    if ((*it)->getRenderOrder() > renderer->getRenderOrder()) {
      break;
    }
    ++it;
  }
  m_subRenderers.insert(it, renderer);
  renderer->setRenderParameters(m_renderParameters);
  if (m_window != nullptr) {
    renderer->initFromParent(m_window);
    renderer->setCameraPose(getCameraPose());
  }

}

void vpPanda3DRendererSet::enableSharedDepthBuffer(vpPanda3DBaseRenderer &sourceBuffer)
{
  for (std::shared_ptr<vpPanda3DBaseRenderer> &subRenderer: m_subRenderers) {
    if (subRenderer.get() != &sourceBuffer) {
      subRenderer->enableSharedDepthBuffer(sourceBuffer);
    }
  }
}

END_VISP_NAMESPACE

#elif !defined(VISP_BUILD_SHARED_LIBS)
// Work around to avoid warning: libvisp_ar.a(vpPanda3DRendererSet.cpp.o) has no symbols
void dummy_vpPanda3DRendererSet() { }

#endif
