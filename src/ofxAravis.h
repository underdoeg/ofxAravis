#pragma once

extern "C"{
#include <arv.h>
}

#include <mutex>
#include <atomic>

#include "ofMain.h"
#include "ofxOpenCv.h"

//template<typename Type>
//class Config{

//	Config():bSet(false){}

//	void set(const Type& val){
//		value = val;
//		bSet = true;
//	}

//	const Type& getValue(){
//		return value;
//	}

//	bool isSet(){
//		return bSet;
//	}

//private:
//	bool bSet;
//	Type value;
//};

class ofxAravis{
public:
	ofxAravis();
	~ofxAravis();

	void setSize(int width, int height);
	void setRegion(int x, int y, int width, int height);
	void setPixelFormat(ArvPixelFormat format);
    void setFrameRate(int fps = 60);
	bool setup(int cam_nr = 0);
	bool isInitialized();
	void stop();

	void setExposure(double exposure);

	void update();

	void draw(int x=0, int y=0, int w=0, int h=0);
    ofTexture& getTexture();

	ArvCamera* camera;
	ArvStream* stream;
    std::vector<std::string> device_IDs;

private:
	static void onNewBuffer(ArvStream *stream, ofxAravis* aravis);
	void setPixels(cv::Mat& mat);

	int targetX, targetY, targetWidth, targetHeight;
	int x, y, width, height;
	ArvPixelFormat targetPixelFormat = ARV_PIXEL_FORMAT_BAYER_RG_8;
	//Config<double> targetExposure, targetGain;
	std::mutex mutex;
	std::atomic_bool bFrameNew;
	ofImage image;
	cv::Mat mat;
};
