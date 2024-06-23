#include "tower_activation.h"
#include "detector.h"


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

SingleFrameEventData BlackWhiteLoadScreenDetector::GetEvent(const cv::Mat& game_img)
{
	SingleFrameEventData ret{ .type = EventType::None };

	// these two bounding boxes are very conservative because many run videos have overlays at the corners
	cv::Rect rect_top = Detector::BBoxConversion<300, 900, 50, 230>(game_img.cols, game_img.rows);
	cv::Rect rect_bottom = Detector::BBoxConversion<480, 950, 370, 600>(game_img.cols, game_img.rows);

	std::array<uint32_t, 256> pixel_count;
	Detector::GreyscaleAccHistogram(game_img(rect_top), pixel_count);
	bool top_all_black = (pixel_count[5] / double(rect_top.area()) > 0.995);
	bool top_all_white = (pixel_count[248] / double(rect_top.area()) < 0.005);
	if (!top_all_black && !top_all_white)
		return { .type = EventType::None };

	Detector::GreyscaleAccHistogram(game_img(rect_bottom), pixel_count);
	bool bottom_all_black = (pixel_count[5] / double(rect_bottom.area()) > 0.995);
	bool bottom_all_white = (pixel_count[248] / double(rect_bottom.area()) < 0.005);

	if (top_all_black)
	{
		if (bottom_all_black)
			return { .type = EventType::BlackScreen };
	}
	else if (top_all_white)
	{
		if (bottom_all_black)
			return { .type = EventType::LoadingScreen };
		else if (bottom_all_white)
			return { .type = EventType::WhiteScreen };
	}

	return { .type = EventType::None };
}

bool AlbumPageDetector::IsOnAlbumPage(const cv::Mat& game_img)
{
	{
		cv::Rect rect_l = Detector::BBoxConversion<482, 497, 32, 46>(game_img.cols, game_img.rows);			// L button
		cv::Rect rect_r = Detector::BBoxConversion<780, 795, 32, 46>(game_img.cols, game_img.rows);			// R button

		std::array<std::array<uint32_t, 256>, 3> pixel_count;

		Detector::BGRAccHistogram(game_img(rect_l), pixel_count);
		if (pixel_count[2][128] / double(rect_l.area()) < 0.9)			// no pixel has red channel > 128
			return false;
		if (pixel_count[1][215] / double(rect_l.area()) < 0.9)			// no pixel has green channel > 215
			return false;
		if (pixel_count[0][150] / double(rect_l.area()) > 0.1)			// most pixels have blue channel > 150
			return false;

		Detector::BGRAccHistogram(game_img(rect_r), pixel_count);
		if (pixel_count[2][128] / double(rect_r.area()) < 0.9)			// no pixel has red channel > 128
			return false;
		if (pixel_count[1][215] / double(rect_r.area()) < 0.9)			// no pixel has green channel > 215
			return false;
		if (pixel_count[0][150] / double(rect_r.area()) > 0.1)			// most pixels have blue channel > 150
			return false;
	}
	{
		cv::Rect rect_left_side = Detector::BBoxConversion<507, 597, 26, 51>(game_img.cols, game_img.rows);		// area between L button and "Album"
		cv::Rect rect_right_side = Detector::BBoxConversion<670, 771, 26, 51>(game_img.cols, game_img.rows);	// area between R button and "Album"
		// nothing brighter than 100 in these areas
		std::array<uint32_t, 256> pixel_count;
		Detector::GreyscaleAccHistogram(game_img(rect_left_side), pixel_count);
		if (pixel_count[100] / double(rect_left_side.area()) < 0.95)
			return false;
		Detector::GreyscaleAccHistogram(game_img(rect_right_side), pixel_count);
		if (pixel_count[100] / double(rect_right_side.area()) < 0.95)
			return false;
	}

	std::array<std::array<uint32_t, 256>, 3> pixel_count;
	cv::Rect rect = Detector::BBoxConversion<600, 669, 26, 51>(game_img.cols, game_img.rows);		// top middle where "Album" is

	Detector::BGRAccHistogram(game_img(rect), pixel_count);

	if (pixel_count[2][128] / double(rect.area()) < 0.9)			// no pixel has red channel > 128
		return false;
	if (pixel_count[1][215] / double(rect.area()) < 0.9)			// no pixel has green channel > 215
		return false;

	double blue_pixel_ratio = 1.0 - pixel_count[0][99] / double(rect.area());
	if (blue_pixel_ratio < 0.45 || blue_pixel_ratio > 0.6)
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(game_img(rect), scale_factor, 85, 170, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Album";
}