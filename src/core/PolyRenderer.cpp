/*
 Copyright (C) 2015 by Ivan Safrin
 
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


#include "polycode/core/PolyRenderer.h"
#include "polycode/core/PolyCoreServices.h"
#include "polycode/core/PolyCore.h"
#include "polycode/core/PolyTexture.h"
#include "polycode/core/PolyVector4.h"

using namespace Polycode;

GraphicsInterface::GraphicsInterface() {
}

RenderThread::RenderThread() : graphicsInterface(NULL) {
    
    rendererShaderBinding = new ShaderBinding();
    
    projectionMatrixParam = rendererShaderBinding->addParam(ProgramParam::PARAM_MATRIX, "projectionMatrix");
    modelMatrixParam = rendererShaderBinding->addParam(ProgramParam::PARAM_MATRIX, "modelMatrix");
    viewMatrixParam = rendererShaderBinding->addParam(ProgramParam::PARAM_MATRIX, "viewMatrix");
    
    for(int i=0; i < RENDERER_MAX_LIGHTS; i++) {
        lights[i].position = rendererShaderBinding->addParam(ProgramParam::PARAM_VECTOR3, "lights["+String::IntToString(i)+"].position");
        lights[i].direction = rendererShaderBinding->addParam(ProgramParam::PARAM_VECTOR3, "lights["+String::IntToString(i)+"].direction");
        lights[i].specular = rendererShaderBinding->addParam(ProgramParam::PARAM_COLOR, "lights["+String::IntToString(i)+"].specular");
        lights[i].diffuse = rendererShaderBinding->addParam(ProgramParam::PARAM_COLOR, "lights["+String::IntToString(i)+"].diffuse");
        lights[i].spotExponent = rendererShaderBinding->addParam(ProgramParam::PARAM_NUMBER, "lights["+String::IntToString(i)+"].spotExponent");
        lights[i].spotCosCutoff = rendererShaderBinding->addParam(ProgramParam::PARAM_NUMBER, "lights["+String::IntToString(i)+"].spotCosCutoff");
        lights[i].constantAttenuation = rendererShaderBinding->addParam(ProgramParam::PARAM_NUMBER, "lights["+String::IntToString(i)+"].constantAttenuation");
        lights[i].linearAttenuation = rendererShaderBinding->addParam(ProgramParam::PARAM_NUMBER, "lights["+String::IntToString(i)+"].linearAttenuation");
        lights[i].quadraticAttenuation = rendererShaderBinding->addParam(ProgramParam::PARAM_NUMBER, "lights["+String::IntToString(i)+"].quadraticAttenuation");
        
    }
    
    jobQueueMutex = Services()->getCore()->createMutex();
}

void RenderThread::runThread() {
    while(threadRunning) {
        
        Services()->getCore()->lockMutex(jobQueueMutex);
        if(jobQueue.size() > 0) {
            RendererThreadJob nextJob = jobQueue.front();
            jobQueue.pop();
            processJob(nextJob);
        }
        Services()->getCore()->unlockMutex(jobQueueMutex);
    }
}

ShaderBinding *RenderThread::getShaderBinding() {
    return rendererShaderBinding;
}

void RenderThread::processDrawBuffer(GPUDrawBuffer *buffer) {
    
    ++currentDebugFrameInfo.buffersProcessed;
    
    if(buffer->targetFramebuffer) {
        graphicsInterface->bindFramebuffer(buffer->targetFramebuffer);
    }
    
    graphicsInterface->setViewport(buffer->viewport.x, buffer->viewport.y, buffer->viewport.w, buffer->viewport.h);
    graphicsInterface->clearBuffers(buffer->clearColor, buffer->clearColorBuffer, buffer->clearDepthBuffer, true);
    
    projectionMatrixParam->setMatrix4(buffer->projectionMatrix);
    viewMatrixParam->setMatrix4(buffer->viewMatrix);

    for(int i=0; i <RENDERER_MAX_LIGHTS; i++) {
        if(i < buffer->lights.size()) {
            lights[i].diffuse->setColor(buffer->lights[i].diffuseColor * buffer->lights[i].intensity);
            lights[i].specular->setColor(buffer->lights[i].specularColor  * buffer->lights[i].intensity);
            
            lights[i].position->setVector3(buffer->lights[i].position);
            lights[i].direction->setVector3(buffer->lights[i].direction);
            lights[i].spotExponent->setNumber(buffer->lights[i].spotlightExponent);
            lights[i].spotCosCutoff->setNumber(buffer->lights[i].spotlightCutoff);
            
            lights[i].constantAttenuation->setNumber(buffer->lights[i].constantAttenuation);
            lights[i].linearAttenuation->setNumber(buffer->lights[i].linearAttenuation);
            lights[i].quadraticAttenuation->setNumber(buffer->lights[i].quadraticAttenuation);
            
        } else {
            lights[i].diffuse->setColor(Color(0.0, 0.0, 0.0, 1.0));
            lights[i].specular->setColor(Color(0.0, 0.0, 0.0, 1.0));
        }
    }
    
    for(int i=0; i < buffer->drawCalls.size(); i++) {
        
        
        if(buffer->drawCalls[i].options.enableScissor) {
            graphicsInterface->enableScissor(true);
            Polycode::Rectangle scissorBox = buffer->drawCalls[i].options.scissorBox;
            
            scissorBox.x *= buffer->backingResolutionScale.x;
            scissorBox.w *= buffer->backingResolutionScale.x;
            scissorBox.h *= buffer->backingResolutionScale.y;
            scissorBox.y = ((buffer->viewport.h*buffer->backingResolutionScale.y)-(scissorBox.y*buffer->backingResolutionScale.y))-scissorBox.h;
            
            graphicsInterface->setScissorBox(scissorBox);
        } else {
            graphicsInterface->enableScissor(false);
        }
        
        graphicsInterface->enableDepthTest(buffer->drawCalls[i].options.depthTest);
        graphicsInterface->enableDepthWrite(buffer->drawCalls[i].options.depthWrite);
        graphicsInterface->enableBackfaceCulling(buffer->drawCalls[i].options.backfaceCull);
        graphicsInterface->setLineSize(buffer->drawCalls[i].options.linePointSize);
        
        modelMatrixParam->setMatrix4(buffer->drawCalls[i].modelMatrix);
        
        Material *material = buffer->drawCalls[i].material;
        if(buffer->globalMaterial && !buffer->drawCalls[i].options.forceMaterial) {
            material = buffer->globalMaterial;
        }
        
        if(material) {
            
            if(buffer->drawCalls[i].options.blendingMode == Renderer::BLEND_MODE_MATERIAL || buffer->globalMaterial) {
                graphicsInterface->setBlendingMode(material->blendingMode);
            } else {
                graphicsInterface->setBlendingMode(buffer->drawCalls[i].options.blendingMode);
            }
            
            
            for(int s=0; s < buffer->drawCalls[i].shaderPasses.size(); s++) {
        
                ++currentDebugFrameInfo.drawCallsProcessed;
                graphicsInterface->beginDrawCall();
                
                ShaderPass shaderPass;
                if(s < material->getNumShaderPasses()) {
                    shaderPass = material->getShaderPass(s);
                } else {
                    shaderPass = buffer->drawCalls[i].shaderPasses[s];
                    graphicsInterface->setBlendingMode(shaderPass.blendingMode);
                }
                ShaderBinding *localShaderBinding = buffer->drawCalls[i].shaderPasses[s].shaderBinding;

                if(!shaderPass.shader || !localShaderBinding) {
                    continue;
                }
                
                graphicsInterface->useShader(shaderPass.shader);

                graphicsInterface->setWireframeMode(shaderPass.wireframe);
                
                ShaderBinding *materialShaderBinding = material->getShaderBinding(s);

                // set global params
                for(int p=0; p < shaderPass.shader->expectedParams.size(); p++) {
                    if(shaderPass.shader->expectedParams[p].globalParam) {
                        graphicsInterface->setParamInShader(shaderPass.shader, &shaderPass.shader->expectedParams[p], shaderPass.shader->expectedParams[p].globalParam);
                    }
                }
                 
                if(materialShaderBinding) {
                    for(int p=0; p < materialShaderBinding->getNumLocalParams(); p++) {                        
                        LocalShaderParam *localParam = materialShaderBinding->getLocalParam(p);
                        if(localParam) {
                            if(!localParam->param) {
                                localParam->param = shaderPass.shader->getParamPointer(localParam->name);
                            }
                            if(localParam->param) {
                                graphicsInterface->setParamInShader(shaderPass.shader, localParam->param, localParam);
                            }
                        }

                    }
                }
                
                for(int p=0; p < localShaderBinding->getNumLocalParams(); p++) {
                    
                    LocalShaderParam *localParam = localShaderBinding->getLocalParam(p);
                    if(localParam) {
                        if(!localParam->param) {
                            localParam->param = shaderPass.shader->getParamPointer(localParam->name);
                        }
                        if(localParam->param) {
                            graphicsInterface->setParamInShader(shaderPass.shader, localParam->param, localParam);
                        }
                    }
                    
                }
                
                for(int a=0; a < localShaderBinding->getNumAttributeBindings(); a++) {
                    
                    AttributeBinding *attributeBinding = localShaderBinding->getAttributeBinding(a);
                    
                    if(attributeBinding) {
                        if(attributeBinding->enabled) {
                            
                            if(!attributeBinding->attribute) {
                                attributeBinding->attribute = shaderPass.shader->getAttribPointer(attributeBinding->name);
                            }
                            if(attributeBinding->attribute) {
                                
                                if(attributeBinding->vertexData->data.size() / attributeBinding->vertexData->countPerVertex >= buffer->drawCalls[i].numVertices) {
                                    graphicsInterface->setAttributeInShader(shaderPass.shader, attributeBinding->attribute, attributeBinding);
                                }
                            } else {
                                attributeBinding->enabled = false;
                            }
                        }
                    }
                    
                }
                
                if(buffer->drawCalls[i].indexed) {
                    graphicsInterface->drawIndices(buffer->drawCalls[i].mode, buffer->drawCalls[i].indexArray);
                } else {
                    graphicsInterface->drawArrays(buffer->drawCalls[i].mode, buffer->drawCalls[i].numVertices);
                }
                
                
                for(int a=0; a < shaderPass.shader->expectedAttributes.size(); a++) {
                    ProgramAttribute attribute = shaderPass.shader->expectedAttributes[a];
                    graphicsInterface->disableAttribute(shaderPass.shader, attribute);
                }
                
                graphicsInterface->endDrawCall();
            }
        }
    }
    
    if(buffer->targetFramebuffer) {
        graphicsInterface->bindFramebuffer(NULL);
    }
    
}

void RenderThread::processJob(const RendererThreadJob &job) {
    switch(job.jobType) {
        case JOB_REQUEST_CONTEXT_CHANGE:
        {
            VideoModeChangeInfo *modeInfo = (VideoModeChangeInfo*) job.data;
            core->handleVideoModeChange(modeInfo);
            delete modeInfo;
        }
        break;
        case JOB_CREATE_TEXTURE:
        {
            Texture *texture = (Texture*) job.data;
            graphicsInterface->createTexture(texture);
        }
        break;
        case JOB_PROCESS_DRAW_BUFFER:
        {
            GPUDrawBuffer *buffer = (GPUDrawBuffer*) job.data;
            processDrawBuffer(buffer);
            delete buffer;
        }
        break;
        case JOB_BEGIN_FRAME:
        {
            currentDebugFrameInfo.buffersProcessed = 0;
            currentDebugFrameInfo.drawCallsProcessed = 0;
            currentDebugFrameInfo.timeTaken = 0;
            frameStart = Services()->getCore()->getTicks();
        }
        break;
        case JOB_END_FRAME:
        {
            core->flushRenderContext();
            currentDebugFrameInfo.timeTaken = Services()->getCore()->getTicks() - frameStart;
            lastFrameDebugInfo = currentDebugFrameInfo;
        }
        break;
        case JOB_CREATE_PROGRAM:
        {
            ShaderProgram *program = (ShaderProgram*) job.data;
            graphicsInterface->createProgram(program);
        }
        break;
        case JOB_CREATE_SHADER:
        {
            Shader *shader = (Shader*) job.data;
            graphicsInterface->createShader(shader);
            
            // set renderer global params
            for(int p=0; p < rendererShaderBinding->getNumLocalParams(); p++) {
                LocalShaderParam *localParam = rendererShaderBinding->getLocalParam(p);
                if(localParam) {
                    ProgramParam *paramPtr = shader->getParamPointer(localParam->name);
                    if(paramPtr) {
                        paramPtr->globalParam = localParam;
                    }
                }
            }
            
            
        }
        break;
        case JOB_CREATE_VERTEX_BUFFERS:
        {
            Mesh *mesh = (Mesh*) job.data;
            
            graphicsInterface->createVertexBuffer(&mesh->vertexPositionArray);
            graphicsInterface->createVertexBuffer(&mesh->vertexTexCoordArray);
            if(mesh->indexedMesh) {
                graphicsInterface->createIndexBuffer(&mesh->indexArray);
            }
        }
        break;
    }
}

RenderThreadDebugInfo RenderThread::getFrameInfo() {
    RenderThreadDebugInfo info;
    Services()->getCore()->lockMutex(jobQueueMutex);
    info = lastFrameDebugInfo;
    Services()->getCore()->unlockMutex(jobQueueMutex);
    return info;
}

void RenderThread::enqueueJob(int jobType, void *data) {
    Services()->getCore()->lockMutex(jobQueueMutex);
    RendererThreadJob job;
    job.jobType = jobType;
    job.data = data;
    jobQueue.push(job);
    Services()->getCore()->unlockMutex(jobQueueMutex);
}

void RenderThread::setGraphicsInterface(Core *core, GraphicsInterface *graphicsInterface) {
    this->graphicsInterface = graphicsInterface;
    this->core = core;
}

Renderer::Renderer() {
    renderThread = new RenderThread();
    Services()->getCore()->createThread(renderThread);
    
    cpuBufferIndex = 0;
    gpuBufferIndex = 1;
}

Renderer::~Renderer() {
    
}

void Renderer::setGraphicsInterface(Core *core, GraphicsInterface *graphicsInterface) {
    renderThread->setGraphicsInterface(core, graphicsInterface);
}

RenderThread *Renderer::getRenderThread() {
    return renderThread;
}

void Renderer::setBackingResolutionScale(Number xScale, Number yScale) {
    backingResolutionScaleX = xScale;
    backingResolutionScaleY = yScale;
}

Number Renderer::getBackingResolutionScaleX() {
    return backingResolutionScaleX;
}

Number Renderer::getBackingResolutionScaleY() {
    return backingResolutionScaleY;
}

Number Renderer::getAnisotropyAmount() {
    return anisotropy;
}

void Renderer::setAnisotropyAmount(Number amount) {
    anisotropy = amount;
}

Cubemap *Renderer::createCubemap(Texture *t0, Texture *t1, Texture *t2, Texture *t3, Texture *t4, Texture *t5) {
    return NULL;
}

void Renderer::processDrawBuffer(GPUDrawBuffer *buffer) {
    buffer->backingResolutionScale.x = backingResolutionScaleX;
    buffer->backingResolutionScale.y = backingResolutionScaleY;
    renderThread->enqueueJob(RenderThread::JOB_PROCESS_DRAW_BUFFER, buffer);
}

void Renderer::beginFrame() {
    renderThread->enqueueJob(RenderThread::JOB_BEGIN_FRAME, NULL);
}

void Renderer::endFrame() {
    renderThread->enqueueJob(RenderThread::JOB_END_FRAME, NULL);
}

Texture *Renderer::createTexture(unsigned int width, unsigned int height, char *textureData, bool clamp, bool createMipmaps, int type, unsigned int filteringMode, unsigned int anisotropy, bool framebufferTexture) {
    Texture *texture = new Texture(width, height, textureData, clamp, createMipmaps, type, framebufferTexture);
    texture->filteringMode = filteringMode;
    texture->anisotropy = anisotropy;
    renderThread->enqueueJob(RenderThread::JOB_CREATE_TEXTURE, (void*)texture);
    return texture;
}

Shader *Renderer::createShader(ShaderProgram *vertexProgram, ShaderProgram *fragmentProgram) {
    Shader *shader = new Shader();
    shader->vertexProgram = vertexProgram;
    shader->fragmentProgram = fragmentProgram;
    renderThread->enqueueJob(RenderThread::JOB_CREATE_SHADER, (void*)shader);
    return shader;
}

ShaderProgram *Renderer::createProgram(const String &fileName) {
    ShaderProgram *program = new ShaderProgram(fileName);
    
    OSFileEntry fileEntry(program->getResourcePath(), OSFileEntry::TYPE_FILE);
    
    if(fileEntry.extension == "vert" ) {
        program->type = ShaderProgram::TYPE_VERT;
    } else {
        program->type = ShaderProgram::TYPE_FRAG;
    }
    
    renderThread->enqueueJob(RenderThread::JOB_CREATE_PROGRAM, (void*)program);
    return program;
}

void Renderer::createVertexBuffers(Mesh *mesh) {
    renderThread->enqueueJob(RenderThread::JOB_CREATE_VERTEX_BUFFERS, (void*) mesh);
}

void Renderer::destroyTexture(Texture *texture) {
    
}

Vector3 Renderer::project(const Vector3 &position, const Matrix4 &modelMatrix, const Matrix4 &projectionMatrix, const Polycode::Rectangle &viewport) {
    
    Vector4 in(position);
    Vector4 out = modelMatrix * in;
    in = projectionMatrix * out;
    
    if (in.w == 0.0) return Vector3();
    
    in.x /= in.w;
    in.y /= in.w;
    in.z /= in.w;
    
    in.x = in.x * 0.5 + 0.5;
    in.y = in.y * 0.5 + 0.5;
    in.z = in.z * 0.5 + 0.5;
    
    in.x = in.x * (viewport.w) + viewport.x;
    in.y = in.y * (viewport.h) + viewport.y;
    
    return Vector3(in.x, in.y, in.z);
}

Vector3 Renderer::unProject(const Vector3 &position, const Matrix4 &modelMatrix, const Matrix4 &projectionMatrix, const Polycode::Rectangle &viewport) {

    Matrix4 finalMatrix = modelMatrix * projectionMatrix;
    finalMatrix = finalMatrix.Inverse();
    
    Vector4 in(position);
    
    in.x = (in.x - viewport.x) / (viewport.w);
    in.y = (in.y - viewport.y) / (viewport.h);
    
    in.y = 1.0 - in.y;
    
    in.x = in.x * 2.0 - 1.0;
    in.y = in.y * 2.0 - 1.0;
    in.z = in.z * 2.0 - 1.0;
    
    Vector4 out = finalMatrix * in;
    
    if(out.w == 0.0) return Vector3();
    
    out.x /= out.w;
    out.y /= out.w;
    out.z /= out.w;
    
    return Vector3(out.x, out.y, out.z);
}
