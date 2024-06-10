#pragma once
#include "common.h"


class TowerActivationDetector
{
private:
	tesseract::TessBaseAPI &_tess_api;

public:
	TowerActivationDetector(tesseract::TessBaseAPI &api);
	~TowerActivationDetector() = default;
	bool Init(const char* lang);

	// returns true if has tower activation dialog
	bool IsActivatingTower(const cv::Mat& game_img);
};


class TravelDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	TravelDetector(tesseract::TessBaseAPI& api);
	~TravelDetector() = default;
	bool Init(const char* lang);

	// returns true if travel button is present
	bool IsTravelButtonPresent(const cv::Mat& game_img);
};
