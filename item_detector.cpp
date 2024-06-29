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
	{ EventType::Paraglider, "Paraglider" },
	{ EventType::ThunderHelm, "Thunder Helm" },
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
	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 0, .brightness_range_upper = 179, .pixel_ratio_lower = 0.45, .pixel_ratio_upper = 1},
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.155, .pixel_ratio_upper = 0.35},
	};
	if (!Detector::GreyscaleTest(game_img(rect_test), crit))
		return EventType::None;

	double scale_factor = 1;
	std::string item_name = Detector::OCR(game_img(rect), scale_factor, 204, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'- ");

	EventType ret = ItemNameToEventType(item_name);

	// link gets thunder helm twice in game, once from Kohga and once from Riju
	// we want to detect the one from Kohga, which has "Inventory" text and a "+" icon at the lower-right corner of the item popup window
	if (ret == EventType::ThunderHelm)
	{
		cv::Rect plus_icon_rect = Detector::BBoxConversion<893, 910, 409, 426>(game_img.cols, game_img.rows);
		static const std::vector<Detector::GreyScaleTestCriteria> crit = {
			{.brightness_range_lower = 180, .brightness_range_upper = 255, .pixel_ratio_lower = 0.5, .pixel_ratio_upper = 1},
		};
		if (!Detector::GreyscaleTest(game_img(plus_icon_rect), crit))
			return EventType::None;
	}
	return ret;
}
