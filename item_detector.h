#pragma once
#include "common.h"


class ItemDetector
{
private:
	tesseract::TessBaseAPI &_tess_api;

private:
	// Lookup the item list and find the best match for the detected item string
	EventType ItemNameToEventType(const std::string& str);

public:
	ItemDetector(tesseract::TessBaseAPI &api);
	~ItemDetector() = default;
	bool Init(const char* lang);

	EventType GetEvent(const cv::Mat& img, const cv::Rect &game_rect);
};
