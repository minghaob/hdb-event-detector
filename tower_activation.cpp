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


TravelDetector::TravelDetector(tesseract::TessBaseAPI& api)
	: _tess_api(api)
{
}

bool TravelDetector::Init(const char* lang)
{
	return true;
}

bool TravelDetector::IsTravelButtonPresent(const cv::Mat& game_img)
{
	cv::Rect rect_left = Detector::BBoxConversion<509, 609, 478, 504>(game_img.cols, game_img.rows);
	cv::Rect rect_middle = Detector::BBoxConversion<610, 672, 478, 504>(game_img.cols, game_img.rows);
	cv::Rect rect_right = Detector::BBoxConversion<673, 771, 478, 504>(game_img.cols, game_img.rows);

	static const std::vector<Detector::GreyScaleTestCriteria> crit_sides = {
		{.brightness_range_lower = 0, .brightness_range_upper = 100, .pixel_ratio_lower = 0.98, .pixel_ratio_upper = 1.0}
	};
	if (!Detector::GreyscaleTest(game_img(rect_left), crit_sides))
		return false;
	if (!Detector::GreyscaleTest(game_img(rect_right), crit_sides))
		return false;
	static const std::vector<Detector::GreyScaleTestCriteria> crit_middle = {
		{.brightness_range_lower = 140, .brightness_range_upper = 255, .pixel_ratio_lower = 0.2, .pixel_ratio_upper = 0.3}
	};
	if (!Detector::GreyscaleTest(game_img(rect_middle), crit_middle))
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(rect_middle), scale_factor, 140, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Travel";
}
