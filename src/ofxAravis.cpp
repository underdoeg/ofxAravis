#include <ofxAravis.h>
#include <opencv2/opencv.hpp>

void ofxAravis::onNewBuffer(ArvStream *stream, ofxAravis *aravis) {
	/*
	ArvBuffer *buffer;

	buffer = arv_stream_try_pop_buffer (stream);
	if (buffer != NULL) {
		if (arv_buffer_get_status (buffer) == ARV_BUFFER_STATUS_SUCCESS){
			auto w = arv_buffer_get_image_width(buffer);
			auto h = arv_buffer_get_image_height(buffer);

			aravis->setPixels(buffer, w, h, ofImageType::OF_IMAGE_GRAYSCALE);

		}
		arv_stream_push_buffer (stream, buffer);
	}
	*/
	ArvBuffer *buffer;

	buffer = arv_stream_try_pop_buffer(stream);
	if (buffer != nullptr) {
		if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS) {
			auto w = arv_buffer_get_image_width(buffer);
			auto h = arv_buffer_get_image_height(buffer);
			auto format = arv_buffer_get_image_pixel_format(buffer);

			cv::Mat matRgb(h, w, CV_8UC3);

			switch (format) {
				case ARV_PIXEL_FORMAT_BAYER_RG_8: {
					cv::Mat matBayer(h, w, CV_8UC1, const_cast<void *>(arv_buffer_get_data(buffer, nullptr)));
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
		arv_stream_push_buffer(stream, buffer);
	}
}

//void ofxAravis::setPixels(ArvBuffer *buffer, int w, int h, ofImageType imageType){
//	this->imageType = imageType;
//	this->w = w;
//	this->h = h;
//	bFrameNew = true;
//	this->buffer = buffer;
//}

void ofxAravis::setPixels(cv::Mat &m) {
	mutex.lock();
	p_last_frame = Clock::now();
	mat = m.clone();
	mutex.unlock();
	bFrameNew = true;
}

////////////////////////////////////////

ofxAravis::ofxAravis() {
	targetX = targetY = 0;
	targetWidth = 1920;
	targetHeight = 1080;
	targetPixelFormat = ARV_PIXEL_FORMAT_BAYER_RG_8;
	p_last_frame = Clock::now();
}

ofxAravis::~ofxAravis() {
	stop();
}

void ofxAravis::setSize(int width, int height) {
	setRegion(0, 0, width, height);
}

void ofxAravis::setRegion(int x, int y, int width, int height) {
	targetX = x;
	targetY = y;
	targetWidth = width;
	targetHeight = height;
}

void ofxAravis::setPixelFormat(ArvPixelFormat format) {
	targetPixelFormat = format;
}

bool ofxAravis::setup() {
	stop();

	bFrameNew = false;

	ofLogNotice("ofxAravis") << "Start grabber";

	GError *err = nullptr;

	camera = arv_camera_new(nullptr, &err);

	if (!camera) {
		ofLogError("ofxAravis") << "No camera found";
		return false;
	}

	arv_camera_set_region(camera, targetX, targetY, targetWidth, targetHeight, &err);

	arv_camera_set_pixel_format(camera, targetPixelFormat, &err);

	arv_camera_get_region(camera, &x, &y, &width, &height, &err);

	image.allocate(width, height, ofImageType::OF_IMAGE_GRAYSCALE);

	auto payload = arv_camera_get_payload(camera, &err);

	stream = arv_camera_create_stream(camera, nullptr, nullptr, &err);
	if (stream != nullptr) {

		// Push 50 buffer in the stream input buffer queue
		for (int i = 0; i < 20; i++)
			arv_stream_push_buffer(stream, arv_buffer_new(payload, nullptr));

		//start stream
		arv_camera_start_acquisition(camera, &err);

		// Connect the new-buffer signal
		g_signal_connect (stream, "new-buffer", G_CALLBACK(onNewBuffer), this);
		arv_stream_set_emit_signals(stream, TRUE);
	} else {
		ofLogError("ofxARavis") << "create stream failed";
	}
	return true;
}

bool ofxAravis::isInitialized() {
	return camera;
}

void ofxAravis::stop() {
	if (!isInitialized())
		return;

	ofLogNotice("ofxAravis") << "Stop";

	arv_stream_set_emit_signals(stream, FALSE);
	GError *err = nullptr;

	arv_camera_stop_acquisition(camera, &err);
	g_object_unref(stream);
	g_object_unref(camera);
}

void ofxAravis::setExposure(double exposure) {
	if (!isInitialized())
		return;
	GError *err = nullptr;

	arv_camera_set_exposure_time_auto(camera, ARV_AUTO_OFF, &err);
	arv_camera_set_exposure_time(camera, exposure, &err);
}

void ofxAravis::update() {
	/*
	if(bFrameNew){
		bFrameNew = false;
		mutex.lock();
        	image.setFromPixels((unsigned char*)(arv_buffer_get_data(buffer, nullptr)), w, h, imageType);
		mutex.unlock();
	}*/
	if (bFrameNew) {
		bFrameNew = false;
		mutex.lock();
		image.setFromPixels(mat.data, width, height, ofImageType::OF_IMAGE_COLOR);
		mutex.unlock();
	}
}

void ofxAravis::draw(int x, int y, int w, int h) {
	if (w == 0) w = width;
	if (h == 0) h = height;
	image.draw(x, y, w, h);
}

ofxAravis::Clock::time_point ofxAravis::last_frame() {
	mutex.lock();
	auto p = p_last_frame;
	mutex.unlock();
	return p;
}

double ofxAravis::getTemperature() {
	if (!camera) return 0;
	ArvDevice *dev = arv_camera_get_device(camera);
	if (!dev) return 0;
	GError *err = nullptr;
	double val = arv_device_get_float_feature_value(dev, "DeviceTemperature", &err);
	if (err) {
		ofLogError("ofxAravis") << "Could not read temperature " << err->message;
		return 0;
	}
	return val;
}