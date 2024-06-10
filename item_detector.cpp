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
	cv::Rect rect = Detector::BBoxConversion<528, 900, 264, 297>(game_img.cols, game_img.rows);

	// Peek the left-most third of the bbox, the items we want to detect are at least this wide
	cv::Rect rect_test = rect;
	rect_test.width /= 3;
	cv::Mat img = game_img(rect_test);
	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 0, .brightness_range_upper = 179, .pixel_ratio_lower = 0.5, .pixel_ratio_upper = 1},
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.155, .pixel_ratio_upper = 0.27},
	};
	if (!Detector::GreyscaleTest(img, crit))
		return "";

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(rect), scale_factor, 204, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'- ");

	return FindBestMatch(ret);
}
