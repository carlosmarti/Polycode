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

#include "polycode/core/PolyCoreServices.h"
#include "polycode/core/PolyCore.h"
#include "polycode/core/PolyCoreInput.h"
#include "polycode/core/PolyInputEvent.h"
#include "polycode/core/PolyLogger.h"
#include "polycode/core/PolyResourceManager.h"
#include "polycode/core/PolyMaterialManager.h"
#include "polycode/core/PolyRenderer.h"
#include "polycode/core/PolyConfig.h"
#include "polycode/core/PolyFontManager.h"
#include "polycode/core/PolySceneManager.h"
#include "polycode/core/PolyTimerManager.h"
#include "polycode/core/PolyTweenManager.h"
#include "polycode/core/PolySoundManager.h"

using namespace Polycode;

std::map<long, CoreServices*> CoreServices::instanceMap;
CoreMutex *CoreServices::renderMutex = 0;
CoreServices* CoreServices::overrideInstance = NULL;

CoreServices *Polycode::Services() {
    return CoreServices::getInstance();
}

CoreMutex *CoreServices::getRenderMutex() {
	if(renderMutex == NULL) {
		Logger::log("Creating render mutex...\n");
		renderMutex = getInstance()->getCore()->createMutex();
	}
	return renderMutex;
}

void CoreServices::setInstance(CoreServices *_instance) {
	overrideInstance = _instance;
	Logger::log("Overriding core instance to %d...\n", _instance);
}

CoreServices* CoreServices::getInstance() {

	if(overrideInstance) {
		return overrideInstance;
	}

//#ifdef _WINDOWS
		overrideInstance = new CoreServices;
		Logger::log("Creating new core services instance...\n");
		return overrideInstance;
//#else
//	long threadID = getThreadID(); 
//	CoreServices *instance;
//	if(instanceMap.find(threadID) == instanceMap.end()) {
//		Logger::log("Creating new core services instance...\n");
//		instance = new CoreServices;
//		instanceMap[threadID] = instance;
//	} else {
//		instance = instanceMap[threadID];
//	}
//	return instance;
//#endif
}

MaterialManager *CoreServices::getMaterialManager() {
	return materialManager;
}

TimerManager *CoreServices::getTimerManager() {
	return timerManager;
}

FontManager *CoreServices::getFontManager() {
	return fontManager;
}

Config *CoreServices::getConfig() {
	return config;
}

Logger *CoreServices::getLogger() {
	return logger;
}


CoreServices::CoreServices() : EventDispatcher() {
	logger = Logger::getInstance();
	resourceManager = new ResourceManager();	
	config = new Config();
	materialManager = new MaterialManager();	
	sceneManager = new SceneManager();
	timerManager = new TimerManager();
	tweenManager = new TweenManager();
	soundManager = new SoundManager();
	fontManager = new FontManager();
}

CoreServices::~CoreServices() {
	delete materialManager;
	delete sceneManager;
	delete timerManager;
	delete tweenManager;
	delete resourceManager;
	delete soundManager;
	delete fontManager;
    delete logger;
    delete config;
    delete renderer;

	instanceMap.clear();
	overrideInstance = NULL;
}

void CoreServices::setCore(Core *core) {
	this->core = core;
}

void CoreServices::handleEvent(Event *event) {
}

Core *CoreServices::getCore() {
	return core;
}

CoreInput *CoreServices::getInput() {
	return core->getInput();
}

void CoreServices::setRenderer(Renderer *renderer) {
	this->renderer = renderer;
	sceneManager->setRenderer(renderer);
}

Renderer *CoreServices::getRenderer() {
	return renderer;
}

void CoreServices::Render(const Polycode::Rectangle &viewport) {

//	renderer->setPerspectiveDefaults();
	sceneManager->renderVirtual();
	sceneManager->Render(viewport);
//	renderer->clearLights();
}

void CoreServices::fixedUpdate() {
    sceneManager->fixedUpdate();
}

void CoreServices::Update(int elapsed) {
	resourceManager->Update(elapsed);
	timerManager->Update();	
	tweenManager->Update(elapsed);	
	materialManager->Update(elapsed);		
	sceneManager->Update();
    soundManager->Update();
}

SoundManager *CoreServices::getSoundManager() {
	return soundManager;
}

SceneManager *CoreServices::getSceneManager() {
	return sceneManager;
}

TweenManager *CoreServices::getTweenManager() {
	return tweenManager;
}

ResourceManager *CoreServices::getResourceManager() {
	return resourceManager;
}

