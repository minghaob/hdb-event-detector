#pragma once
#include "common.h"


class TowerActivationDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	TowerActivationDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~TowerActivationDetector() = default;

	bool Init(const char* lang) {
		return true;
	}

	// returns true if has tower activation dialog
	bool IsActivatingTower(const cv::Mat& game_img);
};


class TravelDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	TravelDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~TravelDetector() = default;
	
	bool Init(const char* lang) {
		return true;
	}

	// returns true if travel button is present
	bool IsTravelButtonPresent(const cv::Mat& game_img);
};

class BlackWhiteLoadScreenDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	BlackWhiteLoadScreenDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~BlackWhiteLoadScreenDetector() = default;

	bool Init(const char* lang) {
		return true;
	}

	// returns true if travel button is present
	SingleFrameEventData GetEvent(const cv::Mat& game_img);
};
