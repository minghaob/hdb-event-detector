#include "item_detector.h"
#include "detector.h"

ItemDetector::ItemDetector(tesseract::TessBaseAPI& api)
	: _tess_api(api)
{
}

bool ItemDetector::Init(const char* lang)
{
	return true;
}

static std::vector<std::pair<EventType, std::string_view>> _items= {
	{ EventType::Korok, "Korok Seed" },
	{ EventType::SpiritOrb, "Spirit Orb" },
	{ EventType::RevaliGale, "Revali's Gale" },
	{ EventType::UrbosaFury, "Urbosa's Fury" },
	{ EventType::MiphaGrace, "Mipha's Grace" },
	{ EventType::DarukProtection, "Daruk's Protection" },
};

EventType ItemDetector::ItemNameToEventType(const std::string& str)
{
	for (const auto& item : _items)
	{
		if (item.second == str)
			return item.first;
	}
	return EventType::None;
}

EventType ItemDetector::GetEvent(const cv::Mat& game_img)
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
		return EventType::None;

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(rect), scale_factor, 204, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'- ");

	return ItemNameToEventType(ret);
}
