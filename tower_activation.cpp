#include "tower_activation.h"
#include "detector.h"


bool TowerActivationDetector::IsActivatingTower(const cv::Mat& img, const cv::Rect& game_rect)
{
	cv::Rect rect = Detector::BBoxConversion<504, 777, 582, 609>(img.cols, img.rows, game_rect);

	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.15, .pixel_ratio_upper = 0.23}
	};
	if (!Detector::GreyscaleTest(img(rect), crit))
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect), scale_factor, 180, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Sheikah Tower activated.";
}

bool SingleLineDialogDetector::Init(const char* lang)
{
	_1line_text_to_npc = {
	};

	for (uint32_t i = 0; i < uint32_t(_1line_text_to_npc.size()); i++)
		util::UnifyAmbiguousChars(_1line_text_to_npc[i].first);

	return true;
}

SingleFrameEventData SingleLineDialogDetector::GetEvent(const cv::Mat& img, const cv::Rect& game_rect)
{
	{
		cv::Rect rect_upper = Detector::BBoxConversion<470, 810, 550, 570>(img.cols, img.rows, game_rect);
		cv::Rect rect_lower = Detector::BBoxConversion<470, 810, 620, 640>(img.cols, img.rows, game_rect);
		static const std::vector<Detector::GreyScaleTestCriteria> crit = {
			{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0, .pixel_ratio_upper = 0.05}
		};
		if (!Detector::GreyscaleTest(img(rect_upper), crit))
			return { .type = EventType::None };
		if (!Detector::GreyscaleTest(img(rect_lower), crit))
			return { .type = EventType::None };
	}

	cv::Rect rect = Detector::BBoxConversion<470, 810, 582, 609>(img.cols, img.rows, game_rect);

	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.1, .pixel_ratio_upper = 0.3}
	};
	if (!Detector::GreyscaleTest(img(rect), crit))
		return { .type = EventType::None };

	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect), scale_factor, 180, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	if (ret == "Travel Gate registered to map.")
		return { .type = EventType::GateRegistered };
	else if (ret == "Sheikah Slate authenticated.")
		return { .type = EventType::SlateAuthenticated };
	else
	{
		for (uint32_t i = 0; i < uint32_t(_1line_text_to_npc.size()); i++)
			if (ret.size() >= _1line_text_to_npc[i].first.size() && util::GetStringEditDistance(std::string_view(ret).substr(0, _1line_text_to_npc[i].first.size()), _1line_text_to_npc[i].first, 4) <= 4)
				return { .type = EventType::Dialog, .dialog_data = {.dialog_id = _1line_text_to_npc[i].second} };
	}
	//else if (ret == "Time has taken its toll on this...")
	//	return { .type = EventType::ZoraMonument, .monument_data = { .monument_id = 8 } };

	return { .type = EventType::None };
}

bool ThreeLineDialogDetector::Init(const char* lang)
{
	_3line_text_to_npc = {
		{"But first you must", DialogId::Kass1},
		{"When a lost hero", DialogId::Kass7},
		{"He's at the Gerudo", DialogId::Canolo},
		{"Ah, I'm glad to hear", DialogId::Oliff},
		{"Hey, if you happen to", DialogId::Sesami1},
		{"I guess I owe you too", DialogId::Perda},
		{"Well, it looks like", DialogId::Dugby},
	};

	for (uint32_t i = 0; i < uint32_t(_3line_text_to_npc.size()); i++)
		util::UnifyAmbiguousChars(_3line_text_to_npc[i].first);

	_2line_text_to_npc = {
		{"When a single arrow", DialogId::Kass2},
		{"Mount the beast", DialogId::Kass3},
		{"On wings of cloth", DialogId::Kass4},
		{"Best of luck, and may", DialogId::Kass5},
		{"Where the forest", DialogId::Kass6},
		{"Good luck figuring", DialogId::Kass8},
		{"If you want to get more", DialogId::Vilia},
		{"To think that there", DialogId::Straia},
		{"You and your horse", DialogId::Jini},
		{"So off we go, me", DialogId::Zyle},
		{"Sesami...is waiting", DialogId::Palme},
		{"Oh, I'm going to go", DialogId::Flaxel},
		{"Not really sure that", DialogId::Sesami2},
		{"Just remember that I", DialogId::Pirou},
		{"She doesn't want to", DialogId::Kheel},
	};

	for (uint32_t i = 0; i < uint32_t(_2line_text_to_npc.size()); i++)
		util::UnifyAmbiguousChars(_2line_text_to_npc[i].first);

	return true;
}

SingleFrameEventData ThreeLineDialogDetector::Get2LineDialogEvent(const cv::Mat& img, const cv::Rect& game_rect)
{
	{
		cv::Rect rect_upper = Detector::BBoxConversion<470, 810, 535, 567>(img.cols, img.rows, game_rect);
		cv::Rect rect_lower = Detector::BBoxConversion<470, 810, 621, 655>(img.cols, img.rows, game_rect);
		static const std::vector<Detector::GreyScaleTestCriteria> crit = {
			{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0, .pixel_ratio_upper = 0.05}
		};
		if (!Detector::GreyscaleTest(img(rect_upper), crit))
			return { .type = EventType::None };
		if (!Detector::GreyscaleTest(img(rect_lower), crit))
			return { .type = EventType::None };
	}

	cv::Rect rect = Detector::BBoxConversion<420, 850, 569, 596>(img.cols, img.rows, game_rect);

	cv::Range clampedXRange = Detector::GreyscaleHorizontalClamp(img(rect), 180, 255);
	if (clampedXRange.size() < rect.width / 3)		// there's too few text to recognize
		return { .type = EventType::None };
	rect.width = clampedXRange.size() + 1;
	rect.x += clampedXRange.start;

	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.1, .pixel_ratio_upper = 0.3}
	};

	if (!Detector::GreyscaleTest(img(rect), crit))
		return { .type = EventType::None };

	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect), scale_factor, 180, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,.'-!\" ");
	util::UnifyAmbiguousChars(ret);

	for (uint32_t i = 0; i < uint32_t(_2line_text_to_npc.size()); i++)
		if (ret.size() >= _2line_text_to_npc[i].first.size() && util::GetStringEditDistance(std::string_view(ret).substr(0, _2line_text_to_npc[i].first.size()), _2line_text_to_npc[i].first, 4) <= 4)
			return { .type = EventType::Dialog, .dialog_data = { .dialog_id = _2line_text_to_npc[i].second} };

	return { .type = EventType::None };
}

SingleFrameEventData ThreeLineDialogDetector::Get3LineDialogEvent(const cv::Mat& img, const cv::Rect& game_rect)
{
	{
		cv::Rect rect_upper = Detector::BBoxConversion<470, 810, 535, 554>(img.cols, img.rows, game_rect);
		cv::Rect rect_lower = Detector::BBoxConversion<470, 810, 636, 655>(img.cols, img.rows, game_rect);
		static const std::vector<Detector::GreyScaleTestCriteria> crit = {
			{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0, .pixel_ratio_upper = 0.05}
		};
		if (!Detector::GreyscaleTest(img(rect_upper), crit))
			return { .type = EventType::None };
		if (!Detector::GreyscaleTest(img(rect_lower), crit))
			return { .type = EventType::None };
	}

	cv::Rect rect = Detector::BBoxConversion<420, 850, 555, 582>(img.cols, img.rows, game_rect);

	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.1, .pixel_ratio_upper = 0.3}
	};
	if (!Detector::GreyscaleTest(img(rect), crit))
		return { .type = EventType::None };

	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect), scale_factor, 180, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,.'-!\" ");
	util::UnifyAmbiguousChars(ret);

	for (uint32_t i = 0; i < uint32_t(_3line_text_to_npc.size()); i++)
		if (ret.size() >= _3line_text_to_npc[i].first.size() && util::GetStringEditDistance(std::string_view(ret).substr(0, _3line_text_to_npc[i].first.size()), _3line_text_to_npc[i].first, 4) <= 4)
			return { .type = EventType::Dialog, .dialog_data = { .dialog_id = _3line_text_to_npc[i].second} };

	return { .type = EventType::None };
}

SingleFrameEventData ThreeLineDialogDetector::GetEvent(const cv::Mat& img, const cv::Rect& game_rect)
{
	SingleFrameEventData ret;
	ret = Get2LineDialogEvent(img, game_rect);
	if (ret.type != EventType::None)
		return ret;

	ret = Get3LineDialogEvent(img, game_rect);
	return ret;
}

bool ZoraMonumentDetector::Init(const char* lang)
{
	_line1_texts = {
		"Our great domain will",
		"Each Zora king since",
		"It was this miracle",
		"And so, keeping to",
		"That the Zora princess",
		"I have spent many long",
		"You can still see the",
		"T--re was a time when",
		"Ever since, the fishers",
		"Still, it is worth it!",
	};

	for (uint32_t i = 0; i < uint32_t(_line1_texts.size()); i++)
		util::UnifyAmbiguousChars(_line1_texts[i]);

	return true;
}

uint8_t ZoraMonumentDetector::GetMonumentID(const cv::Mat& img, const cv::Rect& game_rect)
{
	{
		cv::Rect rect_upper = Detector::BBoxConversion<420, 870, 305, 315>(img.cols, img.rows, game_rect);
		static const std::vector<Detector::GreyScaleTestCriteria> crit = {
			{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0, .pixel_ratio_upper = 0.02}
		};
		if (!Detector::GreyscaleTest(img(rect_upper), crit))
			return 0;
	}

	{
		cv::Rect rect_line1_middle = Detector::BBoxConversion<460, 810, 320, 348>(img.cols, img.rows, game_rect);
		static const std::vector<Detector::GreyScaleTestCriteria> crit = {
			{.brightness_range_lower = 205, .brightness_range_upper = 255, .pixel_ratio_lower = 0.1, .pixel_ratio_upper = 0.3}
		};
		if (!Detector::GreyscaleTest(img(rect_line1_middle), crit))
			return 0;
	}

	cv::Rect rect_line1 = Detector::BBoxConversion<420, 870, 320, 348>(img.cols, img.rows, game_rect);
	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect_line1), scale_factor, 180, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,.-! ");
	util::UnifyAmbiguousChars(ret);

	for (uint32_t i = 0; i < uint32_t(_line1_texts.size()); i++)
		if (ret.size() >= _line1_texts[i].size() && util::GetStringEditDistance(std::string_view(ret).substr(0, _line1_texts[i].size()), _line1_texts[i], 2) <= 2)
			return uint8_t(i + 1);

	return 0;
}

bool TravelDetector::IsTravelButtonPresent(const cv::Mat& img, const cv::Rect& game_rect)
{
	cv::Rect rect_left = Detector::BBoxConversion<509, 609, 478, 504>(img.cols, img.rows, game_rect);
	cv::Rect rect_middle = Detector::BBoxConversion<610, 672, 478, 504>(img.cols, img.rows, game_rect);
	cv::Rect rect_right = Detector::BBoxConversion<673, 771, 478, 504>(img.cols, img.rows, game_rect);

	static const std::vector<Detector::GreyScaleTestCriteria> crit_sides = {
		{.brightness_range_lower = 0, .brightness_range_upper = 100, .pixel_ratio_lower = 0.98, .pixel_ratio_upper = 1.0}
	};
	if (!Detector::GreyscaleTest(img(rect_left), crit_sides))
		return false;
	if (!Detector::GreyscaleTest(img(rect_right), crit_sides))
		return false;
	static const std::vector<Detector::GreyScaleTestCriteria> crit_middle = {
		{.brightness_range_lower = 140, .brightness_range_upper = 255, .pixel_ratio_lower = 0.2, .pixel_ratio_upper = 0.3}
	};
	if (!Detector::GreyscaleTest(img(rect_middle), crit_middle))
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect_middle), scale_factor, 140, 255, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Travel";
}

EventType BlackWhiteLoadScreenDetector::GetEvent(const cv::Mat& img, const cv::Rect& game_rect)
{
	// these two bounding boxes are very conservative because many run videos have overlays at the corners
	cv::Rect rect_top = Detector::BBoxConversion<300, 900, 50, 230>(img.cols, img.rows, game_rect);
	cv::Rect rect_bottom = Detector::BBoxConversion<480, 950, 370, 600>(img.cols, img.rows, game_rect);

	std::array<uint32_t, 256> pixel_count;
	Detector::GreyscaleAccHistogram(img(rect_top), pixel_count);
	bool top_all_black = (pixel_count[9] / double(rect_top.area()) > 0.995);
	bool top_all_white = (pixel_count[246] / double(rect_top.area()) < 0.005);
	if (!top_all_black && !top_all_white)
		return EventType::None;

	Detector::GreyscaleAccHistogram(img(rect_bottom), pixel_count);
	bool bottom_all_black = (pixel_count[9] / double(rect_bottom.area()) > 0.995);
	bool bottom_all_white = (pixel_count[246] / double(rect_bottom.area()) < 0.005);

	if (top_all_black)
	{
		if (bottom_all_black)
			return EventType::BlackScreen;
	}
	else if (top_all_white)
	{
		if (bottom_all_black)
			return EventType::LoadingScreen;
		else if (bottom_all_white)
			return EventType::WhiteScreen;
	}

	return EventType::None;
}

bool AlbumPageDetector::IsOnAlbumPage(const cv::Mat& img, const cv::Rect& game_rect)
{
	{
		cv::Rect rect_l = Detector::BBoxConversion<482, 497, 32, 46>(img.cols, img.rows, game_rect);			// L button
		cv::Rect rect_r = Detector::BBoxConversion<780, 795, 32, 46>(img.cols, img.rows, game_rect);			// R button

		std::array<std::array<uint32_t, 256>, 3> pixel_count;

		Detector::BGRAccHistogram(img(rect_l), pixel_count);
		if (pixel_count[2][128] / double(rect_l.area()) < 0.9)			// no pixel has red channel > 128
			return false;
		if (pixel_count[1][230] / double(rect_l.area()) < 0.9)			// no pixel has green channel > 230
			return false;
		if (pixel_count[0][150] / double(rect_l.area()) > 0.15)			// most pixels have blue channel > 150
			return false;

		Detector::BGRAccHistogram(img(rect_r), pixel_count);
		if (pixel_count[2][128] / double(rect_r.area()) < 0.9)			// no pixel has red channel > 128
			return false;
		if (pixel_count[1][230] / double(rect_r.area()) < 0.9)			// no pixel has green channel > 230
			return false;
		if (pixel_count[0][150] / double(rect_r.area()) > 0.15)			// most pixels have blue channel > 150
			return false;
	}
	{
		cv::Rect rect_left_side = Detector::BBoxConversion<507, 597, 26, 51>(img.cols, img.rows, game_rect);		// area between L button and "Album"
		cv::Rect rect_right_side = Detector::BBoxConversion<670, 771, 26, 51>(img.cols, img.rows, game_rect);		// area between R button and "Album"
		// nothing brighter than 100 in these areas
		std::array<uint32_t, 256> pixel_count;
		Detector::GreyscaleAccHistogram(img(rect_left_side), pixel_count);
		if (pixel_count[100] / double(rect_left_side.area()) < 0.95)
			return false;
		Detector::GreyscaleAccHistogram(img(rect_right_side), pixel_count);
		if (pixel_count[100] / double(rect_right_side.area()) < 0.95)
			return false;
	}

	std::array<std::array<uint32_t, 256>, 3> pixel_count;
	cv::Rect rect = Detector::BBoxConversion<600, 669, 26, 51>(img.cols, img.rows, game_rect);		// top middle where "Album" is

	Detector::BGRAccHistogram(img(rect), pixel_count);

	if (pixel_count[2][128] / double(rect.area()) < 0.9)			// no pixel has red channel > 128
		return false;
	if (pixel_count[1][230] / double(rect.area()) < 0.9)			// no pixel has green channel > 230
		return false;

	double blue_pixel_ratio = pixel_count[0][99] / double(rect.area());
	double blue_pixel_ratio2 = pixel_count[0][179] / double(rect.area());
	if (blue_pixel_ratio < 0.4 || blue_pixel_ratio > 0.55 || blue_pixel_ratio2 > 0.9)
		return false;

	double scale_factor = 1;
	std::string ret = Detector::OCR(img(rect), scale_factor, 85, 170, true, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. ");

	return ret == "Album";
}