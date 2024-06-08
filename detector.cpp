#include "detector.h"

std::string Detector::OCR(const cv::Mat& input, double scale_factor, uint8_t greyscale_lower, uint8_t greyscale_upper, tesseract::TessBaseAPI& tess_api, const char* char_whitelist)
{
	cv::Mat bbox_frame;
	cv::resize(input, bbox_frame, cv::Size(int(input.cols / scale_factor), int(input.rows / scale_factor)));

	cv::cvtColor(bbox_frame, bbox_frame, cv::COLOR_BGR2GRAY);
	for (int i = 0; i < bbox_frame.rows; i++)
	{
		uint8_t lower = greyscale_lower;
		uint8_t upper = greyscale_upper;
		uint8_t* data = bbox_frame.row(i).data;
		for (int j = 0; j < bbox_frame.cols; j++)
			data[j] = 255 - uint8_t(uint32_t(std::clamp(data[j], lower, upper) - lower) * 255 / (upper - lower));
	}
	cv::cvtColor(bbox_frame, bbox_frame, cv::COLOR_GRAY2BGRA);

	// reorder the channels in each pixel for leptonica
	util::OpenCvMatBGRAToLeptonicaRGBAInplace(bbox_frame);

	// construct the PIX struct
	PIX* pix = pixCreateHeader(bbox_frame.cols, bbox_frame.rows, 32);
	pixSetDimensions(pix, bbox_frame.cols, bbox_frame.rows, 32);
	pixSetWpl(pix, bbox_frame.cols);
	pixSetSpp(pix, 4);
	pixSetData(pix, (l_uint32*)bbox_frame.data);

	// OCR
	if (!tess_api.SetVariable("tessedit_char_whitelist", char_whitelist))
		return "";

	tess_api.SetImage(pix);
	tess_api.Recognize(0);

	pixSetData(pix, nullptr);
	pixDestroy(&pix);

	std::string ret = std::unique_ptr<char[]>(tess_api.GetUTF8Text()).get();

	// OCR text from tesseract sometimes ends with '\n', trim that
	if (ret.size() && ret[ret.size() - 1] == '\n')
		ret = ret.substr(0, ret.size() - 1);

	return ret;
}

bool Detector::GreyscaleTest(const cv::Mat& img, const std::vector<GreyScaleTestCriteria> &criteria)
{
	cv::Mat minimalFrame;
	cv::cvtColor(img, minimalFrame, cv::COLOR_BGR2GRAY);		// converting to gray

	// scan the image
	std::vector<uint32_t> numPixel(criteria.size(), 0);
	for (int i = 0; i < minimalFrame.rows; i++)
	{
		uint8_t* data = minimalFrame.row(i).data;
		for (int j = 0; j < minimalFrame.cols; j++)
		{
			for (size_t k = 0; k < criteria.size(); k++)
			{
				if (data[j] >= criteria[k].brightness_range_lower && data[j] <= criteria[k].brightness_range_upper)
					numPixel[k]++;
			}
		}
	}

	// check pixel ratio
	for (size_t i = 0; i < criteria.size(); i++)
	{
		double pixel_ratio = double(numPixel[i]) / (minimalFrame.rows * minimalFrame.cols);
		if (pixel_ratio < criteria[i].pixel_ratio_lower || pixel_ratio > criteria[i].pixel_ratio_upper)
			return false;
	}

	return true;
}
