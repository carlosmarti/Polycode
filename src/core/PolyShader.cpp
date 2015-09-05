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

#include "polycode/core/PolyShader.h"
#include "polycode/core/PolyMatrix4.h"
#include "polycode/core/PolyCoreServices.h"

using namespace Polycode;

ShaderRenderTarget::ShaderRenderTarget() : PolyBase() {
	texture = NULL;
}

ProgramParam::ProgramParam() : type(PARAM_UNKNOWN), platformData(NULL), globalParam(NULL) {
}

ProgramAttribute::ProgramAttribute() : platformData(NULL) {
    
}

void *ProgramParam::createParamData(int type) {
	switch (type) {
		case PARAM_NUMBER:
		{
			Number *val = new Number();
			return (void*)val;
		}
		break;
		case PARAM_VECTOR2:
		{
			Vector2 *val = new Vector2();
			return (void*)val;
		}
		break;
		case PARAM_VECTOR3:
		{
			Vector3 *val = new Vector3();
			return (void*)val;
		}
		break;		
		case PARAM_COLOR:
		{
			Color *val = new Color();
			return (void*)val;
		}
		break;		
		case PARAM_MATRIX:
		{
			Matrix4 *val = new Matrix4();
			return (void*)val;
		}
		break;		
		default:		
			return NULL;
		break;
	}
}

ShaderProgram::ShaderProgram(const String &fileName) : Resource(Resource::RESOURCE_PROGRAM) {
    setResourcePath(fileName);
    setResourceName(fileName);
}

ShaderProgram::~ShaderProgram() {

}

void ShaderProgram::reloadResource() {
	reloadProgram();
	Resource::reloadResource();	
}


ShaderBinding::ShaderBinding() {
}

ShaderBinding::~ShaderBinding() {
	for(int i=0; i < localParams.size(); i++) {
		delete localParams[i];
	}	
	for(int i=0; i < renderTargetBindings.size(); i++) {
		delete renderTargetBindings[i];
	}	
}

unsigned int ShaderBinding::getNumLocalParams() {
	return localParams.size();
}

unsigned int ShaderBinding::getNumAttributeBindings() {
    return attributes.size();
}

AttributeBinding *ShaderBinding::getAttributeBinding(unsigned int index) {
    return attributes[index];
}

LocalShaderParam *ShaderBinding::getLocalParam(unsigned int index) {
	return localParams[index];
}

LocalShaderParam *ShaderBinding::getLocalParamByName(const String& name) {
	for(int i=0; i < localParams.size(); i++) {
		if(localParams[i]->name == name) {
			return localParams[i];
		}
	}
	return NULL;
}

AttributeBinding *ShaderBinding::getAttributeBindingByName(const String &name) {
    for(int i=0; i < attributes.size(); i++) {
        if(attributes[i]->name == name) {
            return attributes[i];
        }
    }
    return NULL;
}


LocalShaderParam * ShaderBinding::addParam(int type, const String& name) {
	void *defaultData = ProgramParam::createParamData(type);
	LocalShaderParam *newParam = new LocalShaderParam();
	newParam->data = defaultData;
	newParam->name = name;
    newParam->type = type;
    newParam->param = NULL;
	localParams.push_back(newParam);
	return newParam;
}

LocalShaderParam *ShaderBinding::addParamPointer(int type, const String& name, void *ptr) {
    LocalShaderParam *newParam = new LocalShaderParam();
    newParam->name = name;
    newParam->data = ptr;
    newParam->type = type;
    newParam->param = NULL;
    newParam->ownsPointer = false;
    localParams.push_back(newParam);
    return newParam;
}

void ShaderBinding::addRenderTargetBinding(RenderTargetBinding *binding) {
	renderTargetBindings.push_back(binding);
	switch (binding->mode) {
		case RenderTargetBinding::MODE_IN:
			inTargetBindings.push_back(binding);		
		break;
		case RenderTargetBinding::MODE_OUT:
			outTargetBindings.push_back(binding);		
		break;
		case RenderTargetBinding::MODE_COLOR:
			colorTargetBindings.push_back(binding);		
		break;
		case RenderTargetBinding::MODE_DEPTH:
			depthTargetBindings.push_back(binding);		
		break;				
	}
}

void ShaderBinding::removeRenderTargetBinding(RenderTargetBinding *binding) {
	for(int i=0; i < renderTargetBindings.size(); i++) {
		if(renderTargetBindings[i] == binding) {
			renderTargetBindings.erase(renderTargetBindings.begin() + i);
		}
	}
	
	for(int i=0; i < inTargetBindings.size(); i++) {
		if(inTargetBindings[i] == binding) {
			inTargetBindings.erase(inTargetBindings.begin() + i);
			return;
		}
	}
	for(int i=0; i < outTargetBindings.size(); i++) {
		if(outTargetBindings[i] == binding) {
			outTargetBindings.erase(outTargetBindings.begin() + i);
			return;
		}
	}
	
	for(int i=0; i < colorTargetBindings.size(); i++) {
		if(colorTargetBindings[i] == binding) {
			colorTargetBindings.erase(colorTargetBindings.begin() + i);
			return;
		}
	}

	for(int i=0; i < depthTargetBindings.size(); i++) {
		if(depthTargetBindings[i] == binding) {
			depthTargetBindings.erase(depthTargetBindings.begin() + i);
			return;
		}
	}
}

void ShaderBinding::copyTo(ShaderBinding *targetBinding) {
    for(int i=0; i < localParams.size(); i++) {
        LocalShaderParam *copyParam = localParams[i]->Copy();
        targetBinding->localParams.push_back(copyParam);
    }
}

unsigned int ShaderBinding::getNumRenderTargetBindings() {
	return renderTargetBindings.size();
}

RenderTargetBinding *ShaderBinding::getRenderTargetBinding(unsigned int index) {
	return renderTargetBindings[index];
}

unsigned int ShaderBinding::getNumInTargetBindings() {
	return inTargetBindings.size();
}

RenderTargetBinding *ShaderBinding::getInTargetBinding(unsigned int index) {
	return inTargetBindings[index];
}

unsigned int ShaderBinding::getNumOutTargetBindings() {
	return outTargetBindings.size();
}

RenderTargetBinding *ShaderBinding::getOutTargetBinding(unsigned int index) {
	return outTargetBindings[index];
}

unsigned int ShaderBinding::getNumColorTargetBindings() {
	return colorTargetBindings.size();
}

RenderTargetBinding *ShaderBinding::getColorTargetBinding(unsigned int index) {
	return colorTargetBindings[index];
}

unsigned int ShaderBinding::getNumDepthTargetBindings() {
	return depthTargetBindings.size();
}

RenderTargetBinding *ShaderBinding::getDepthTargetBinding(unsigned int index) {
	return depthTargetBindings[index];
}

Texture *ShaderBinding::loadTextureForParam(const String &paramName, const String &fileName) {
    Texture *texture = Services()->getMaterialManager()->createTextureFromFile(fileName);
    setTextureForParam(paramName, texture);
    return texture;
}

void ShaderBinding::setTextureForParam(const String &paramName, Texture *texture) {
    if(!texture) {
        removeParam(paramName);
        return;
    }
    LocalShaderParam *textureParam = getLocalParamByName(paramName);
    if(!textureParam) {
        textureParam = addParam(ProgramParam::PARAM_TEXTURE, paramName);
    }
    textureParam->setTexture(texture);
}

void ShaderBinding::setCubemapForParam(const String &paramName, Cubemap *cubemap) {
    LocalShaderParam *textureParam = getLocalParamByName(paramName);
    if(!textureParam) {
        textureParam = addParam(ProgramParam::PARAM_CUBEMAP, paramName);
    }
    textureParam->setCubemap(cubemap);
}

void ShaderBinding::removeParam(const String &name) {
    for(int i=0; i < localParams.size(); i++) {
        if(localParams[i]->name == name) {
            delete localParams[i];
            localParams.erase(localParams.begin()+i);
        }
    }
}

Shader::Shader() : Resource(Resource::RESOURCE_SHADER) {
	numSpotLights = 0;
	numPointLights = 0;
	vertexProgram = NULL;
	fragmentProgram = NULL;
}

ProgramParam *Shader::getParamPointer(const String &name) {
    for(int i=0; i < expectedParams.size(); i++) {
        if(expectedParams[i].name == name) {
            return &expectedParams[i];
        }
    }
    return NULL;
}

ProgramAttribute *Shader::getAttribPointer(const String &name) {
    for(int i=0; i < expectedAttributes.size(); i++) {
        if(expectedAttributes[i].name == name) {
            return &expectedAttributes[i];
        }
    }
    return NULL;
}


int Shader::getExpectedParamType(String name) {
	for(int i=0; i < expectedParams.size(); i++) {
		if(expectedParams[i].name == name) {
			return expectedParams[i].type;
		}
	}
	return ProgramParam::PARAM_UNKNOWN;
}

Shader::~Shader() {

}

int Shader::getType() const {
	return type;
}

void Shader::setName(const String& name) {
	this->name = name;
}

Number LocalShaderParam::getNumber() {
    if(type != ProgramParam::PARAM_NUMBER) {
        return 0.0;
    }
    return *((Number *)data);
}

Vector2 LocalShaderParam::getVector2() {
    if(type != ProgramParam::PARAM_VECTOR2) {
        return Vector2();
    }
    return *((Vector2 *)data);
}

Vector3 LocalShaderParam::getVector3() {
    if(type != ProgramParam::PARAM_VECTOR3) {
        return Vector3();
    }
    return *((Vector3 *)data);
}

Matrix4 LocalShaderParam::getMatrix4() {
    if(type != ProgramParam::PARAM_MATRIX) {
        return Matrix4();
    }
    return *((Matrix4 *)data);
}

Color LocalShaderParam::getColor() {
    if(type != ProgramParam::PARAM_COLOR) {
        return Color(0.0, 0.0, 0.0, 0.0);
    }
    return *((Color *)data);
}

void LocalShaderParam::setNumber(Number x) {
    if(type != ProgramParam::PARAM_NUMBER) {
        return;
    }
    memcpy(data, &x, sizeof(x));
}

void LocalShaderParam::setVector2(Vector2 x) {
    if(type != ProgramParam::PARAM_VECTOR2) {
        return;
    }
    memcpy(data, &x, sizeof(x));
}

void LocalShaderParam::setVector3(Vector3 x) {
    if(type != ProgramParam::PARAM_VECTOR3) {
        return;
    }
    memcpy(data, &x, sizeof(x));
}

void LocalShaderParam::setMatrix4(Matrix4 x) {
    if(type != ProgramParam::PARAM_MATRIX) {
        return;
    }
    memcpy(data, &x, sizeof(x));
}

void LocalShaderParam::setColor(Color x) {
    if(type != ProgramParam::PARAM_COLOR) {
        return;
    }
    static_cast<Color*>(data)->setColor(&x);
}

const String& Shader::getName() const {
	return name;
}

LocalShaderParam::LocalShaderParam() {
    data = NULL;
    arraySize = 0;
    ownsPointer = true;
}

void LocalShaderParam::setTexture(Texture *texture) {
    data = (void*) texture;
}

Texture *LocalShaderParam::getTexture() {
    return (Texture*) data;
}

void LocalShaderParam::setCubemap(Cubemap *cubemap) {
    data = (void*) cubemap;
}

Cubemap *LocalShaderParam::getCubemap() {
    return (Cubemap*) data;
}

LocalShaderParam::~LocalShaderParam() {
    if(ownsPointer) {
        switch(type) {
            case ProgramParam::PARAM_NUMBER:
                delete ((Number*) data);
            break;
            case ProgramParam::PARAM_VECTOR2:
                delete ((Vector2*) data);
            break;
            case ProgramParam::PARAM_VECTOR3:
                delete ((Vector3*) data);
            break;
            case ProgramParam::PARAM_COLOR:
                delete ((Color*) data);
            break;
            case ProgramParam::PARAM_MATRIX:
                delete ((Matrix4*) data);
            break;
        }
    }
}

LocalShaderParam *LocalShaderParam::Copy() {
    LocalShaderParam *copyParam = new LocalShaderParam();
    copyParam->name = name;
    copyParam->type = type;
    copyParam->data = ProgramParam::createParamData(type);
    copyParam->ownsPointer = ownsPointer;
    
    switch(type) {
        case ProgramParam::PARAM_NUMBER:
        {
            copyParam->setNumber(getNumber());
        }
        break;
        case ProgramParam::PARAM_VECTOR2:
        {
            copyParam->setVector2(getVector2());
        }
        break;
        case ProgramParam::PARAM_VECTOR3:
        {
            copyParam->setVector3(getVector3());
        }
        break;
        case ProgramParam::PARAM_COLOR:
        {
            copyParam->setColor(getColor());
        }
        break;
        case ProgramParam::PARAM_MATRIX:
        {
            copyParam->setMatrix4(getMatrix4());
        }
        break;
    }
    return copyParam;
}

AttributeBinding::AttributeBinding() : enabled(true), vertexData(NULL), attribute(NULL) {
    
}

AttributeBinding *ShaderBinding::addAttributeBinding(const String &name, VertexDataArray *dataArray) {
    AttributeBinding *binding = new AttributeBinding();
    binding->name = name;
    binding->attribute = NULL;
    binding->vertexData = dataArray;
    attributes.push_back(binding);
    return binding;
}

void LocalShaderParam::setParamValueFromString(int type, String pvalue) {
        switch(type) {
            case ProgramParam::PARAM_NUMBER:
            {
                setNumber(atof(pvalue.c_str()));
            }
                break;
            case ProgramParam::PARAM_VECTOR2:
            {
                std::vector<String> values = pvalue.split(" ");
                if(values.size() == 2) {
                    setVector2(Vector2(atof(values[0].c_str()), atof(values[1].c_str())));
                } else {
					printf("Material parameter error: Vector2 %s must have 2 values (%d provided)!\n", name.c_str(), (int)values.size());
                }
            }
                break;
            case ProgramParam::PARAM_VECTOR3:
            {
                std::vector<String> values = pvalue.split(" ");
                if(values.size() == 3) {
                    setVector3(Vector3(atof(values[0].c_str()), atof(values[1].c_str()), atof(values[2].c_str())));
                } else {
					printf("Material parameter error: Vector3 %s must have 3 values (%d provided)!\n", name.c_str(), (int)values.size());
                }
            }
                break;
            case ProgramParam::PARAM_COLOR:
            {
                std::vector<String> values = pvalue.split(" ");
                if(values.size() == 4) {
                    setColor(Color(atof(values[0].c_str()), atof(values[1].c_str()), atof(values[2].c_str()), atof(values[3].c_str())));
                } else {
					printf("Material parameter error: Color %s must have 4 values (%d provided)!\n", name.c_str(), (int)values.size());
                }
            }
                break;
        }
}
