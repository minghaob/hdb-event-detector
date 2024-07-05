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
	static void GreyscaleAccHistogram(const cv::Mat& img, std::array<uint32_t, 256> &pix_count);
	static void BGRAccHistogram(const cv::Mat& img, std::array<std::array<uint32_t, 256>, 3>& pix_count);
	static std::string OCR(const cv::Mat& input, double scale_factor, uint8_t greyscale_lower, uint8_t greyscale_upper, bool invert_color, tesseract::TessBaseAPI& tess_api, const char* char_whitelist);

	template<uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, uint32_t orig_width = 1280, uint32_t orig_height = 720>
	static cv::Rect BBoxConversion(uint32_t width, uint32_t height, const cv::Rect& rect)
	{
		constexpr double bbox_x0 = left / (double)orig_width;
		constexpr double bbox_x1 = right / (double)orig_width;
		constexpr double bbox_y0 = top / (double)orig_height;
		constexpr double bbox_y1 = bottom / (double)orig_height;

		// get the bounding box in rows / cols
		uint32_t bbox_col0 = uint32_t(bbox_x0 * double(rect.width) + 0.5);
		uint32_t bbox_col1 = uint32_t(bbox_x1 * double(rect.width) + 0.5);
		uint32_t bbox_row0 = uint32_t(bbox_y0 * double(rect.height) + 0.5);
		uint32_t bbox_row1 = uint32_t(bbox_y1 * double(rect.height) + 0.5);
		if (bbox_col0 + rect.x < 0 || bbox_row0 + rect.y < 0 || bbox_col0 + rect.x + bbox_col1 - bbox_col0 > width || bbox_row0 + rect.y + bbox_row1 - bbox_row0 > height)
		{
			std::cout << "BBoxConversion result outside image" << std::endl;
			exit(-1);
		}
		return cv::Rect(bbox_col0 + rect.x, bbox_row0 + rect.y, bbox_col1 - bbox_col0, bbox_row1 - bbox_row0);
	}
};
