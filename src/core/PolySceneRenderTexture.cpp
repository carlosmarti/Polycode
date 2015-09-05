/*
 Copyright (C) 2011 by Ivan Safrin
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include "polycode/core/PolySceneRenderTexture.h"
#include "polycode/core/PolyCoreServices.h"
#include "polycode/core/PolyRenderer.h"
#include "polycode/core/PolySceneManager.h"
#include "polycode/core/PolyTexture.h"
#include "polycode/core/PolyScene.h"
#include "polycode/core/PolyCamera.h"

using namespace Polycode;

SceneRenderTexture::SceneRenderTexture(Scene *targetScene, Camera *targetCamera, int renderWidth,int renderHeight, unsigned int textureFormat) : textureFormat(textureFormat) {
    
    targetTexture = Services()->getRenderer()->createTexture(renderWidth, renderHeight, NULL, false, false, textureFormat, Services()->getMaterialManager()->getTextureFilteringMode(), 0, true);
    
	this->targetScene = targetScene;
	this->targetCamera = targetCamera;

	CoreServices::getInstance()->getSceneManager()->registerRenderTexture(this);
    renderer = Services()->getRenderer();
    
	enabled = true;
}

void SceneRenderTexture::resizeRenderTexture(int newWidth, int newHeight) {

	if(newWidth > 0 && newHeight > 0) {
		CoreServices::getInstance()->getRenderer()->destroyTexture(targetTexture);
		CoreServices::getInstance()->getRenderer()->destroyTexture(depthTexture);	
		CoreServices::getInstance()->getRenderer()->destroyTexture(filterColorBufferTexture);
		CoreServices::getInstance()->getRenderer()->destroyTexture(filterZBufferTexture);	

        targetTexture = Services()->getRenderer()->createTexture(newWidth, newHeight, NULL, false, false, textureFormat, Services()->getMaterialManager()->getTextureFilteringMode(), 0, true);

	}
}
	
Scene *SceneRenderTexture::getTargetScene() {
	return targetScene;
}

Texture *SceneRenderTexture::getFilterColorBufferTexture() {
	return filterColorBufferTexture;
}

Texture *SceneRenderTexture::getFilterZBufferTexture() {
	return filterZBufferTexture;
}

Camera *SceneRenderTexture::getTargetCamera() {
	return targetCamera;
}

void SceneRenderTexture::Render() {
    
//    if(targetCamera->hasFilterShader()) {
       // targetCamera->drawFilter(targetTexture, targetTexture->getWidth(), targetTexture->getHeight(), filterColorBufferTexture, filterZBufferTexture);
  //  } else {
        targetCamera->setViewport(Polycode::Rectangle(0.0, 0.0, targetTexture->getWidth(), targetTexture->getHeight()));
        targetScene->Render(targetCamera, targetTexture);
   // }
}

Image *SceneRenderTexture::saveToImage() {
    //RENDERER_TODO
    //return renderer->renderBufferToImage(targetTexture);
    return NULL;
}

Texture *SceneRenderTexture::getTargetTexture() {
	return targetTexture;
}

SceneRenderTexture::~SceneRenderTexture() {
	CoreServices::getInstance()->getSceneManager()->unregisterRenderTexture(this);
	CoreServices::getInstance()->getRenderer()->destroyTexture(targetTexture);
	CoreServices::getInstance()->getRenderer()->destroyTexture(depthTexture);	
	CoreServices::getInstance()->getRenderer()->destroyTexture(filterColorBufferTexture);
	CoreServices::getInstance()->getRenderer()->destroyTexture(filterZBufferTexture);	
}
