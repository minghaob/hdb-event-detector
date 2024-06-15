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

IntraFrameEvent BlackWhiteLoadScreenDetector::GetEvent(const cv::Mat& game_img)
{
	IntraFrameEvent ret{ .type = IntraFrameEventType::None };

	// these two bounding boxes are very conservative because many run videos have overlays at the corners
	cv::Rect rect_top = Detector::BBoxConversion<300, 950, 50, 250>(game_img.cols, game_img.rows);
	cv::Rect rect_bottom = Detector::BBoxConversion<480, 950, 320, 600>(game_img.cols, game_img.rows);

	std::array<uint32_t, 256> pixel_count;
	Detector::GreyscaleAccHistogram(game_img(rect_top), pixel_count);
	bool top_all_black = (pixel_count[5] == uint32_t(rect_top.area()));
	bool top_all_white = (pixel_count[248] == 0);
	if (!top_all_black && !top_all_white)
		return { .type = IntraFrameEventType::None };

	Detector::GreyscaleAccHistogram(game_img(rect_bottom), pixel_count);
	bool bottom_all_black = (pixel_count[5] == uint32_t(rect_bottom.area()));
	bool bottom_all_white = (pixel_count[248] == 0);

	if (top_all_black)
	{
		if (bottom_all_black)
			return { .type = IntraFrameEventType::BlackScreen };
	}
	else if (top_all_white)
	{
		if (bottom_all_black)
			return { .type = IntraFrameEventType::LoadingScreen };
		else if (bottom_all_white)
			return { .type = IntraFrameEventType::WhiteScreen };
	}

	return { .type = IntraFrameEventType::None };
}