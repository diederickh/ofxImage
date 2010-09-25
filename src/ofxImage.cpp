/*
 *  ofxImage.cpp
 *
 *  Created by Pat Long (plong0) on 08/06/10.
 *  Copyright 2010 Tangible Interaction Inc. All rights reserved.
 *
 */
#include "ofxImage.h"

//------------------------------------
ofxImage::ofxImage():ofImage(){
	fileName = "";
}

//------------------------------------
void ofxImage::clone(ofxImage &mom){
	fileName = mom.getFileName();
	ofImage::clone(mom);
}

//------------------------------------
void ofxImage::clearPixels(int x, int y, int w, int h, int r, int g, int b, int a){
	unsigned char* pixels = this->getPixels();
	int Bpp = this->bpp/8;
	for(int i=x; i < x+w && i < this->width; i++){
		for(int j=y; j < y+h && j < this->height; j++){
			pixels[(j*this->width+i)*Bpp+0] = r;
			if(Bpp > 1){
				pixels[(j*this->width+i)*Bpp+1] = g;
				if(Bpp > 2){
					pixels[(j*this->width+i)*Bpp+2] = b;
					if(Bpp > 3){
						pixels[(j*this->width+i)*Bpp+3] = a;
					}
				}
			}
		}
	}
	this->setFromPixels(pixels, this->width, this->height, this->type);
}

//------------------------------------
unsigned char * ofxImage::getPixels(){
	return ofImage::getPixels();
}

//------------------------------------
unsigned char * ofxImage::getPixels(unsigned char* cropPixels, int x, int y, int w, int h){
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;
	if(cropPixels == NULL)
		cropPixels = new unsigned char[w*h*this->bpp/8];
	unsigned char* pixels = this->getPixels();
	int Bpp = this->bpp / 8;
	for (int i = x; i < (x+w) && i < this->width; i++){
		for (int j = y; j < (y+h) && j < this->height; j++){
			for(int b = 0; b < Bpp; b++)
				cropPixels[((j-y)*w+(i-x))*Bpp+b] = pixels[(j*this->width+i)*Bpp+b];
		}
	}
	return cropPixels;
}

//------------------------------------
string ofxImage::getFileName(){
	return fileName;
}

//------------------------------------
bool ofxImage::loadImage(string fileName){
	string localFileName = getFileName();
	if(fileName == "" && localFileName != "")
		fileName = localFileName;
	bool result = ofImage::loadImage(fileName);
	if(result && fileName != getFileName())
		setFileName(fileName);
	return result;
}

//------------------------------------
bool ofxImage::saveImage(string fileName) {
	string localFileName = getFileName();
	if(fileName == "" && localFileName != "")
		fileName = localFileName;

	bool result = saveImageFromPixels(fileName, myPixels);

	if(result && fileName != getFileName())
		setFileName(fileName);

	return result;
}

//----------------------------------------------------------------
// copied directly from core ofImage::saveImageFromPixels, with added bool return value
bool ofxImage::saveImageFromPixels(string fileName, ofPixels &pix, int nQuality){
	bool result = false;
	if (pix.bAllocated == false){
		ofLog(OF_LOG_ERROR,"error saving image - pixels aren't allocated");
		return result;
	}

#ifdef TARGET_LITTLE_ENDIAN
	if (pix.bytesPerPixel != 1) swapRgb(pix);
#endif

	FIBITMAP * bmp	= getBmpFromPixels(pix);

#ifdef TARGET_LITTLE_ENDIAN
	if (pix.bytesPerPixel != 1) swapRgb(pix);
#endif

	fileName = ofToDataPath(fileName);
	if (pix.bAllocated == true){
		FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
		fif = FreeImage_GetFileType(fileName.c_str(), 0);
		if(fif == FIF_UNKNOWN) {
			// or guess via filename
			fif = FreeImage_GetFIFFromFilename(fileName.c_str());
		}
		if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
			result = FreeImage_Save(fif, bmp, fileName.c_str(), nQuality);
		}
	}

	if (bmp != NULL){
		FreeImage_Unload(bmp);
	}
	return result;
}


//------------------------------------
void ofxImage::setFileName(string fileName){
	this->fileName = fileName;
}

//------------------------------------
void ofxImage::mirror(bool horizontal, bool vertical){
	flipPixels(myPixels, horizontal, vertical);

	update();
}

//------------------------------------
void ofxImage::rotate(float angle){
	rotatePixels(myPixels, angle);

	tex.clear();
	if (bUseTexture == true){
		tex.allocate(myPixels.width, myPixels.height, myPixels.glDataType);
	}

	update();
}

//----------------------------------------------------
void ofxImage::flipPixels(ofPixels &pix, bool horizontal, bool vertical){
	if(!horizontal && !vertical)
		return;

	FIBITMAP * bmp               = getBmpFromPixels(pix);
	bool horSuccess = false, vertSuccess = false;

	if(horizontal)
		horSuccess = FreeImage_FlipHorizontal(bmp);
	if(vertical)
		vertSuccess = FreeImage_FlipVertical(bmp);

	if(horSuccess || vertSuccess)
		putBmpIntoPixels(bmp, pix);

	if (bmp != NULL)            FreeImage_Unload(bmp);
}

//----------------------------------------------------
void ofxImage::rotatePixels(ofPixels &pix, float angle){
	if(angle == 0.0)
		return;

	FIBITMAP * bmp               = getBmpFromPixels(pix);
	FIBITMAP * convertedBmp         = NULL;

	convertedBmp = FreeImage_RotateClassic(bmp, angle);
	putBmpIntoPixels(convertedBmp, pix);

	if (bmp != NULL)            FreeImage_Unload(bmp);
	if (convertedBmp != NULL)      FreeImage_Unload(convertedBmp);
}

//----------------------------------------------------
void ofxImage::loadFromURL(string sURL) {
	cout << " load....:  "<< sURL << std::endl;
	if(sURL == "")
		return;
	//poco is not happy if we register the factory more than once
   if(!factoryLoaded){
      HTTPStreamFactory::registerFactory();
      factoryLoaded = true;
   }

//copy to our string
   string str;
   //specify out url and open stream
	try {
		URI uri(sURL);
		std::auto_ptr<std::istream> pStr(URIStreamOpener::defaultOpener().open(uri));
	   StreamCopier::copyToString(*pStr.get(), str);
	}
	catch (Poco::Exception& e) {
		std::cout << "Poco thrown exception while loading the image." << std::endl;
		return;
	}





   //figure out how many bytes the image is and allocate
   int bytesToRead = str.size();
   char* buff = new char[bytesToRead];
   memset(buff, 0, bytesToRead);

   //copy the bytes from the string to our buffer
   for(int i = 0; i < bytesToRead; i++){
    buff[i] = str[i];
   }

   printf("numBytes copied is %i \n", sizeof(buff));


//++
	FIMEMORY *hmem = NULL;
    hmem = FreeImage_OpenMemory((unsigned char*)buff, bytesToRead);
	if (hmem == NULL){
		printf("couldn't create memory handle! \n");
		return;
	}

	//get the file type!
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem);
	if( fif == -1 ){
		printf("unable to guess format", fif);
		return;
		FreeImage_CloseMemory(hmem);
	}

	//make the image!!
	FIBITMAP * tmpBmp = FreeImage_LoadFromMemory(fif, hmem, 0);
	/*
	if( rotateMode > 0 && rotateMode < 4){
		FIBITMAP * oldBmp = tmpBmp;

		if( rotateMode == 1)tmpBmp = FreeImage_RotateClassic(tmpBmp, 90);
		if( rotateMode == 2)tmpBmp = FreeImage_RotateClassic(tmpBmp, 180);
		if( rotateMode == 3)tmpBmp = FreeImage_RotateClassic(tmpBmp, 270);

		FreeImage_Unload(oldBmp);
	}
	*/
	//FreeImage_FlipVertical(tmpBmp);
	printf("LOADED!");
	putBmpIntoPixels(tmpBmp, myPixels);
	width 		= FreeImage_GetWidth(tmpBmp);
	height 		= FreeImage_GetHeight(tmpBmp);
	bpp 		= FreeImage_GetBPP(tmpBmp);

	//swapRgb(myPixels);
	cout << " loaded:  "<< sURL << std::endl;
	//setFromPixels(getPixels(), width,height, OF_IMAGE_COLOR);
	FreeImage_Unload(tmpBmp);
	FreeImage_CloseMemory(hmem);
	update();
//++



   //if we already have a loaded image clear it
 //  if(isValid()){
   // clear();
   //}

   /*
   //create a freeimage memory handle from the buffer address
   FIMEMORY *hmem = NULL;
   hmem = FreeImage_OpenMemory((unsigned char *)buff, bytesToRead);
   if (hmem == NULL){ printf("couldn't create memory handle! \n"); return; }

   //get the file type!
   FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem);

   //make the image!!
   bmp = FreeImage_LoadFromMemory(fif, hmem, 0);

   //free our memory
   FreeImage_CloseMemory(hmem);

   if (bmp == NULL){ printf("couldn't create bmp! \n"); return; }

   //flip it!
   FreeImage_FlipVertical(bmp);
   update();
   */
}

// The swapRgb function isn't defined in the openFrameworks.lib
// on Windows/codeblocks
//------------------------------------------------------------
inline void ofxImage::swapRgb(ofPixels &pix){
	if (pix.bitsPerPixel != 8){
		int sizePixels		= pix.width*pix.height;
		int cnt				= 0;
		unsigned char temp;
		int byteCount		= pix.bitsPerPixel/8;

		while (cnt < sizePixels){
			temp					= pix.pixels[cnt*byteCount];
			pix.pixels[cnt*byteCount]		= pix.pixels[cnt*byteCount+2];
			pix.pixels[cnt*byteCount+2]		= temp;
			cnt++;
		}
	}
}

