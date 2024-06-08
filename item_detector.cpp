#include "item_detector.h"
#include "detector.h"

ItemDetector::ItemDetector(tesseract::TessBaseAPI& api)
	: _tess_api(api)
{
}

bool ItemDetector::Init(const char* lang)
{
	if (!InitItemList(lang))
		return false;

	return true;
}

bool ItemDetector::InitItemList(const char* lang)
{
	_items.emplace_back("Korok Seed");
	_items.emplace_back("Spirit Orb");

	return true;
}

std::string ItemDetector::FindBestMatch(const std::string& str)
{
	for (const auto& item : _items)
	{
		if (item == str)
			return str;
	}
	return "";
}

std::string ItemDetector::GetItem(const cv::Mat& game_img)
{
	// This is the bounding box of the longest location text in the lower left corner of the game screen.
	constexpr double bbox_x0 = 528 / (double)1280;
	constexpr double bbox_x1 = 900 / (double)1280;
	constexpr double bbox_y0 = 264 / (double)720;
	constexpr double bbox_y1 = 297 / (double)720;

	// get the bounding box in rows / cols
	uint32_t bbox_col0 = uint32_t(bbox_x0 * double(game_img.cols) + 0.5);
	uint32_t bbox_col1 = uint32_t(bbox_x1 * double(game_img.cols) + 0.5);
	uint32_t bbox_row0 = uint32_t(bbox_y0 * double(game_img.rows) + 0.5);
	uint32_t bbox_row1 = uint32_t(bbox_y1 * double(game_img.rows) + 0.5);

	// Peek the left-most third of the bbox, the items we want to detect are at least this this wide
	cv::Mat img = game_img(cv::Rect(bbox_col0, bbox_row0, (bbox_col1 - bbox_col0) / 3, bbox_row1 - bbox_row0));
	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 0, .brightness_range_upper = 179, .pixel_ratio_lower = 0.5, .pixel_ratio_upper = 1},
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.155, .pixel_ratio_upper = 0.27},
	};
	if (!Detector::GreyscaleTest(img, crit))
		return "";

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(cv::Rect(bbox_col0, bbox_row0, bbox_col1 - bbox_col0, bbox_row1 - bbox_row0)), scale_factor, 204, 255, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'- ");

	return FindBestMatch(ret);
}
