#pragma once
#include "common.h"


class ItemDetector
{
private:
	tesseract::TessBaseAPI _tess_api;
	std::vector<std::string> _items;

private:
	bool InitItemList(const char* lang);

	// Lookup the item list and find the best match for the detected item string
	std::string FindBestMatch(const std::string& str);

public:
	ItemDetector() = default;
	~ItemDetector();
	bool Init(const char* lang);

	// returns empty string if nothing is detected
	std::string GetItem(const cv::Mat& game_img);
};