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