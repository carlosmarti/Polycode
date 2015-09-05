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

#include "string.h"
#include "polycode/core/PolyTexture.h"
#include <stdlib.h>

using namespace Polycode;

Texture::Texture(unsigned int width, unsigned int height, char *textureData,bool clamp, bool createMipmaps, int type, bool framebufferTexture) : Resource(Resource::RESOURCE_TEXTURE), width(width), height(height), clamp(clamp), type(type), createMipmaps(createMipmaps), filteringMode(FILTERING_NEAREST), anisotropy(0), framebufferTexture(framebufferTexture), frameBufferPlatformData(NULL) {
    
	switch(type) {
		case Image::IMAGE_RGB:
			pixelSize = 3;			
			break;
		case Image::IMAGE_RGBA:
			pixelSize = 4;						
		break;
		case Image::IMAGE_FP16:		
			pixelSize = 12;
		break;
		default:
			pixelSize = 4;								
		break;
	}
	
    if(!framebufferTexture) {
        this->textureData = (char*)malloc(width*height*pixelSize);
        if(textureData)
            memcpy(this->textureData, textureData, width*height*pixelSize);	
        else
            memset(this->textureData, 0, width*height*pixelSize);
    } else {
        this->textureData = NULL;
    }

}

void Texture::reloadResource() {
	Image *image = new Image(getResourcePath());
	setImageData(image);
	delete image;
	Resource::reloadResource();	
}

int Texture::getWidth() const {
	return width;
}

int Texture::getHeight() const {
	return height;
}

Texture::~Texture(){
	free(textureData);
}

void Texture::setImageData(Image *data) {

	switch (data->getType()) {
		case Image::IMAGE_RGB:
			pixelSize = 3;			
		break;
		case Image::IMAGE_RGBA:
			pixelSize = 4;						
		break;
		case Image::IMAGE_FP16:		
			pixelSize = 12;
		break;
		default:
			pixelSize = 4;								
		break;
	}

	width = data->getWidth();
	height = data->getHeight();
	
	if(this->textureData)
		free(this->textureData);
	this->textureData = (char*)malloc(width*height*pixelSize);
	memcpy(this->textureData, data->getPixels(), width*height*pixelSize);

}

Texture::Texture(Image *image) : Resource(Resource::RESOURCE_TEXTURE) {	
	pixelSize = 4;
	this->textureData = (char*)malloc(image->getWidth()*image->getHeight()*pixelSize);
	memcpy(this->textureData, image->getPixels(), image->getWidth()*image->getHeight()*pixelSize);	

}
