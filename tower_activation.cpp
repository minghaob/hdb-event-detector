#include "tower_activation.h"

bool TowerActivationDetector::Init(const char* lang)
{
	if (_tess_api.Init(".", lang))
	{
		std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
		return false;
	}

	// items are always on one line
	_tess_api.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);

	if (std::string_view(lang) == "eng")
	{
		if (!_tess_api.SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz. "))
			return false;
	}
	// ignore extra space at the end of the line without any text, doesn't seem to make much difference though
	if (!_tess_api.SetVariable("gapmap_use_ends", "true"))
		return false;

	return true;
}

static bool EarlyOutTest(const cv::Mat& game_img, uint32_t bbox_col0, uint32_t bbox_col1, uint32_t bbox_row0, uint32_t bbox_row1)
{
	int _brightness_threshold = 204;
	double _bright_pixel_ratio_low = 0.15, _bright_pixel_ratio_high = 0.23;

	// Peek the left-most third of the bbox, the items we want to detect are at least this this wide
	cv::Mat locationMinimalFrame;
	cv::cvtColor(game_img(cv::Rect(bbox_col0, bbox_row0, (bbox_col1 - bbox_col0), bbox_row1 - bbox_row0)), locationMinimalFrame, cv::COLOR_BGR2GRAY);		// converting to gray
	//cv::imwrite("tower.png", locationMinimalFrame);

	// scan this area for bright pixels.
	{
		uint32_t num_bright_pixel = 0;
		for (int i = 0; i < locationMinimalFrame.rows; i++)
		{
			uint8_t* data = locationMinimalFrame.row(i).data;
			for (int j = 0; j < locationMinimalFrame.cols; j++)
				if (data[j] > _brightness_threshold)
					num_bright_pixel++;
		}
		double bright_pixel_ratio = double(num_bright_pixel) / (locationMinimalFrame.rows * locationMinimalFrame.cols);
		//std::cout << bright_pixel_ratio << std::endl;
		if (bright_pixel_ratio < _bright_pixel_ratio_low || bright_pixel_ratio > _bright_pixel_ratio_high)
			return true;
	}

	return false;
}

bool TowerActivationDetector::IsActivatingTower(const cv::Mat& game_img)
{
	// This is the bounding box of the longest location text in the lower left corner of the game screen.
	constexpr double bbox_x0 = 504 / (double)1280;
	constexpr double bbox_x1 = 777 / (double)1280;
	constexpr double bbox_y0 = 582 / (double)720;
	constexpr double bbox_y1 = 609 / (double)720;

	// get the bounding box in rows / cols
	uint32_t bbox_col0 = uint32_t(bbox_x0 * double(game_img.cols) + 0.5);
	uint32_t bbox_col1 = uint32_t(bbox_x1 * double(game_img.cols) + 0.5);
	uint32_t bbox_row0 = uint32_t(bbox_y0 * double(game_img.rows) + 0.5);
	uint32_t bbox_row1 = uint32_t(bbox_y1 * double(game_img.rows) + 0.5);

	if (EarlyOutTest(game_img, bbox_col0, bbox_col1, bbox_row0, bbox_row1))
		return false;

	// shrink the whole bbox frame to make OCR faster
	cv::Mat bbox_frame;
	double scale_factor = 1;// std::max(game_img.cols / 640.0, 1.0);	// according to experiments, it's still possible to recognize the item with high accuracy when the width of the game screen is 480.
	cv::resize(game_img(cv::Rect(bbox_col0, bbox_row0, bbox_col1 - bbox_col0, bbox_row1 - bbox_row0)), bbox_frame, cv::Size(int((bbox_col1 - bbox_col0) / scale_factor), int((bbox_row1 - bbox_row0) / scale_factor)));

	cv::cvtColor(bbox_frame, bbox_frame, cv::COLOR_BGR2GRAY);
	for (int i = 0; i < bbox_frame.rows; i++)
	{
		uint8_t lower = 180;
		uint8_t upper = 255;
		uint8_t* data = bbox_frame.row(i).data;
		for (int j = 0; j < bbox_frame.cols; j++)
			data[j] = uint8_t(((std::clamp(data[j], lower, upper) -  lower) * 255.0 + 0.5) / (upper - lower));
	}
	cv::cvtColor(bbox_frame, bbox_frame, cv::COLOR_GRAY2BGRA);
	//cv::imwrite("item.png", bbox_frame);

	util::OpenCvMatBGRAToLeptonicaRGBAInplace(bbox_frame);

	// construct the PIX struct
	PIX* pix = pixCreateHeader(bbox_frame.cols, bbox_frame.rows, 32);
	pixSetDimensions(pix, bbox_frame.cols, bbox_frame.rows, 32);
	pixSetWpl(pix, bbox_frame.cols);
	pixSetSpp(pix, 4);
	pixSetData(pix, (l_uint32*)bbox_frame.data);

	// OCR
	_tess_api.SetImage(pix);
	_tess_api.Recognize(0);

	pixSetData(pix, nullptr);
	pixDestroy(&pix);

	std::string ret = std::unique_ptr<char[]>(_tess_api.GetUTF8Text()).get();

	// OCR text from tesseract sometimes ends with '\n', trim that
	if (ret.size() && ret[ret.size() - 1] == '\n')
		ret = ret.substr(0, ret.size() - 1);

	//std::cout << ret << std::endl;
	return ret == "Sheikah Tower activated.";
}

TowerActivationDetector::~TowerActivationDetector()
{
	_tess_api.Clear();
}