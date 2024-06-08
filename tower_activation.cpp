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
	// This is the bounding box of the longest location text in the lower left corner of the game screen.
	constexpr double bbox_x0 = 504 / (double)1280;
	constexpr double bbox_x1 = 777 / (double)1280;
	constexpr double bbox_y0 = 582 / (double)720;
	constexpr double bbox_y1 = 609 / (double)720;

	// get the bounding box in rows / cols
	uint32_t bbox_col0 = uint32_t(bbox_x0 * double(game_img.cols) + 0.5);
	uint32_t bbox_col1 = uint32_t(bbox_x1 * double(game_img.cols) + 0.5);
	uint32_t bbox_row0 = uint32_t(bbox_y0 * double(game_img.rows) + 0.5);
	uint32_t bbox_row1 = uint32_t(bbox_y1 * double(game_img.rows) + 0.5);

	cv::Mat img = game_img(cv::Rect(bbox_col0, bbox_row0, bbox_col1 - bbox_col0, bbox_row1 - bbox_row0));
	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.15, .pixel_ratio_upper = 0.23}
	};
	if (!Detector::GreyscaleTest(img, crit))
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(cv::Rect(bbox_col0, bbox_row0, bbox_col1 - bbox_col0, bbox_row1 - bbox_row0)), scale_factor, 180, 255, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Sheikah Tower activated.";
}
