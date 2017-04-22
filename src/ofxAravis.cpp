#include <ofxAravis.h>
#include <opencv2/opencv.hpp>

void ofxAravis::onNewBuffer(ArvStream *stream, ofxAravis* aravis){
	ArvBuffer *buffer;

	buffer = arv_stream_try_pop_buffer (stream);
	if (buffer != NULL) {
		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS){
			auto w = arv_buffer_get_image_width(buffer);
			auto h = arv_buffer_get_image_height(buffer);
			auto format = arv_buffer_get_image_pixel_format(buffer);

			cv::Mat matRgb(h, w, CV_8UC3);

			switch(format){
			case ARV_PIXEL_FORMAT_BAYER_RG_8:
			{
				cv::Mat matBayer(h, w, CV_8UC1, const_cast<void*>(arv_buffer_get_data(buffer, nullptr)));
				cv::cvtColor(matBayer, matRgb, CV_BayerRG2BGR);
			}
				break;
			default:
				ofLogError("ofxARavis") << "Unknown pixel format";
			}

			//cv::Mat matBayer(h, w, CV_8UC1, const_cast<void*>(arv_buffer_get_data(buffer, nullptr)));
			//cv::Mat matRgb(h, w, CV_8UC3);
			//cv::cvtColor(matBayer, matRgb, CV_BayerRG2BGR);

			// flip it
			//cv::Mat matRgbFlip = matRgb.clone();
			//cv::flip(matRgb, matRgbFlip, 1);

			aravis->setPixels(matRgb);
		}
		arv_stream_push_buffer (stream, buffer);
	}
}

void ofxAravis::setPixels(cv::Mat &m){
	mutex.lock();
	mat = m.clone();
	mutex.unlock();
	bFrameNew = true;
}


////////////////////////////////////////

ofxAravis::ofxAravis(){
	targetX = targetY = 0;
	targetWidth = 1920;
	targetHeight = 1080;
	targetPixelFormat = ARV_PIXEL_FORMAT_BAYER_RG_8;
}

ofxAravis::~ofxAravis(){
	stop();
}

void ofxAravis::setSize(int width, int height){
	setRegion(0, 0, width, height);
}

void ofxAravis::setRegion(int x, int y, int width, int height){
	targetX = x;
	targetY = y;
	targetWidth = width;
	targetHeight = height;
}

void ofxAravis::setPixelFormat(ArvPixelFormat format){
	targetPixelFormat = format;
}

bool ofxAravis::setup(){
	stop();

	bFrameNew = false;

	ofLogNotice("ofxAravis") << "Start grabber";

	camera = arv_camera_new(nullptr);

	if(!camera){
		ofLogError("ofxAravis") << "No camera found";
		return false;
	}

	arv_camera_set_region(camera, targetX, targetY, targetWidth, targetHeight);

	arv_camera_set_pixel_format(camera, targetPixelFormat);

	arv_camera_get_region(camera, &x, &y, &width, &height);

	image.allocate(width, height, ofImageType::OF_IMAGE_COLOR);

	auto payload = arv_camera_get_payload (camera);

	stream = arv_camera_create_stream (camera, nullptr, nullptr);
	if (stream != nullptr) {

		// Push 50 buffer in the stream input buffer queue
		for (int i = 0; i < 20; i++)
			arv_stream_push_buffer (stream, arv_buffer_new (payload, nullptr));

		//start stream
		arv_camera_start_acquisition (camera);

		// Connect the new-buffer signal
		g_signal_connect (stream, "new-buffer", G_CALLBACK (onNewBuffer), this);
		arv_stream_set_emit_signals (stream, TRUE);
	}

	return true;
}

bool ofxAravis::isInitialized(){
	return camera;
}

void ofxAravis::stop(){
	if(!isInitialized())
		return;

	ofLogNotice("ofxAravis") << "Stop";

	arv_stream_set_emit_signals(stream, FALSE);

	arv_camera_stop_acquisition(camera);
	g_object_unref(stream);
	g_object_unref(camera);
}

void ofxAravis::setExposure(double exposure){
	if(!isInitialized())
		return;

	arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF);
	arv_camera_set_exposure_time(camera, exposure);
}

void ofxAravis::update(){
	if(bFrameNew){
		bFrameNew = false;
		mutex.lock();
		image.setFromPixels(mat.data, width, height, ofImageType::OF_IMAGE_COLOR);
		mutex.unlock();
	}
}

void ofxAravis::draw(int x, int y, int w, int h){
	if(w == 0) w = width;
	if(h == 0) h = height;
	image.draw(x, y, w, h);
}
