#include "tower_activation.h"
#include "detector.h"

TowerActivationDetector::TowerActivationDetector(tesseract::TessBaseAPI& api)
	: _tess_api(api)
{
}

bool TowerActivationDetector::Init(const char* lang)
{
	return true;
}

bool TowerActivationDetector::IsActivatingTower(const cv::Mat& game_img)
{
	cv::Rect rect = Detector::BBoxConversion<504, 777, 582, 609>(game_img.cols, game_img.rows);

	cv::Mat img = game_img(rect);
	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.15, .pixel_ratio_upper = 0.23}
	};
	if (!Detector::GreyscaleTest(img, crit))
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(rect), scale_factor, 180, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Sheikah Tower activated.";
}
