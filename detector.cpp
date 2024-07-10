#include "detector.h"

std::string Detector::OCR(const cv::Mat& input, double scale_factor, uint8_t greyscale_lower, uint8_t greyscale_upper, bool invert_color, tesseract::TessBaseAPI& tess_api, const char* char_whitelist)
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
			data[j] = uint8_t(uint32_t(std::clamp(data[j], lower, upper) - lower) * 255 / (upper - lower));
		if (invert_color)
		{
			for (int j = 0; j < bbox_frame.cols; j++)
				data[j] = 255 - data[j];
		}
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

void Detector::GreyscaleAccHistogram(const cv::Mat& img, std::array<uint32_t, 256> &pix_count)
{
	cv::Mat grey_image;
	cv::cvtColor(img, grey_image, cv::COLOR_BGR2GRAY);		// converting to gray

	pix_count.fill(0);
	for (int i = 0; i < grey_image.rows; i++)
	{
		uint8_t* data = grey_image.row(i).data;
		for (int j = 0; j < grey_image.cols; j++)
			pix_count[data[j]]++;
	}
	for (int i = 1; i <= 255; i++)
		pix_count[i] += pix_count[i - 1];
}

cv::Range Detector::GreyscaleHorizontalClamp(const cv::Mat& img, uint8_t brightness_lower, uint8_t brightness_upper)
{
	cv::Mat grey_image;
	cv::cvtColor(img, grey_image, cv::COLOR_BGR2GRAY);		// converting to gray

	int32_t left = img.cols;
	int32_t right = -1;

	for (int i = 0; i < grey_image.rows; i++)
	{
		uint8_t* data = grey_image.row(i).data;
		for (int j = 0; j < grey_image.cols; j++)
			if (data[j] >= brightness_lower && data[j] <= brightness_upper)
			{
				left = std::min(left, j);
				right = std::max(right, j);
			}
	}

	return cv::Range(left, right);
}

void Detector::BGRAccHistogram(const cv::Mat& img, std::array<std::array<uint32_t, 256>, 3>& pix_count)
{
	pix_count[0].fill(0);
	pix_count[1].fill(0);
	pix_count[2].fill(0);
	for (int i = 0; i < img.rows; i++)
	{
		uint8_t* data = img.row(i).data;
		for (int j = 0; j < img.cols; j++)
		{
			pix_count[0][data[j * 3]]++;
			pix_count[1][data[j * 3 + 1]]++;
			pix_count[2][data[j * 3 + 2]]++;
		}
	}
	for (int i = 1; i <= 255; i++)
	{
		pix_count[0][i] += pix_count[0][i - 1];
		pix_count[1][i] += pix_count[1][i - 1];
		pix_count[2][i] += pix_count[2][i - 1];
	}
}

bool Detector::GreyscaleTest(const cv::Mat& img, const std::vector<GreyScaleTestCriteria> &criteria)
{
	// scan the image
	std::array<uint32_t, 256> count;
	GreyscaleAccHistogram(img, count);

	// check pixel ratio
	for (size_t i = 0; i < criteria.size(); i++)
	{
		uint32_t num_pixel = count[criteria[i].brightness_range_upper];
		if (criteria[i].brightness_range_lower > 0)
			num_pixel -= count[criteria[i].brightness_range_lower - 1];
		double pixel_ratio = double(num_pixel) / (img.rows * img.cols);
		if (pixel_ratio < criteria[i].pixel_ratio_lower || pixel_ratio > criteria[i].pixel_ratio_upper)
			return false;
	}

	return true;
}

