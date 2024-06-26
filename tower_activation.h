#pragma once
#include "common.h"


class TowerActivationDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	TowerActivationDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~TowerActivationDetector() = default;

	bool Init(const char* lang) {
		return true;
	}

	bool IsActivatingTower(const cv::Mat& game_img);
};


class SingleLineDialogDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	SingleLineDialogDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~SingleLineDialogDetector() = default;

	bool Init(const char* lang) {
		return true;
	}

	EventType GetEvent(const cv::Mat& game_img);
};


class TravelDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	TravelDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~TravelDetector() = default;
	
	bool Init(const char* lang) {
		return true;
	}

	bool IsTravelButtonPresent(const cv::Mat& game_img);
};

class BlackWhiteLoadScreenDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	BlackWhiteLoadScreenDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~BlackWhiteLoadScreenDetector() = default;

	bool Init(const char* lang) {
		return true;
	}

	EventType GetEvent(const cv::Mat& game_img);
};

class AlbumPageDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;

public:
	AlbumPageDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~AlbumPageDetector() = default;

	bool Init(const char* lang) {
		return true;
	}

	bool IsOnAlbumPage(const cv::Mat& game_img);
};