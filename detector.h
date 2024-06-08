#pragma once
#include <string>
#include <vector>
#include "common.h"


class Detector
{
public:
	struct GreyScaleTestCriteria {
		uint8_t brightness_range_lower;
		uint8_t brightness_range_upper;
		double pixel_ratio_lower;
		double pixel_ratio_upper;
	};
public:
	static bool GreyscaleTest(const cv::Mat& img, const std::vector<GreyScaleTestCriteria>& criteria);
	static std::string OCR(const cv::Mat& input, double scale_factor, uint8_t greyscale_lower, uint8_t greyscale_upper, tesseract::TessBaseAPI& tess_api, const char* char_whitelist);
};
