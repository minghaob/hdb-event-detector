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

	bool IsActivatingTower(const cv::Mat& img, const cv::Rect& game_rect);
};


class SingleLineDialogDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;
	std::vector<std::pair<std::string, DialogId>> _1line_text_to_npc;

public:
	SingleLineDialogDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~SingleLineDialogDetector() = default;

	bool Init(const char* lang);

	SingleFrameEventData GetEvent(const cv::Mat& img, const cv::Rect& game_rect);
};

class ThreeLineDialogDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;
	std::vector<std::pair<std::string, DialogId>> _3line_text_to_npc;
	std::vector<std::pair<std::string, DialogId>> _2line_text_to_npc;

public:
	ThreeLineDialogDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~ThreeLineDialogDetector() = default;

	bool Init(const char* lang);

	SingleFrameEventData Get2LineDialogEvent(const cv::Mat& img, const cv::Rect& game_rect);
	SingleFrameEventData Get3LineDialogEvent(const cv::Mat& img, const cv::Rect& game_rect);
	SingleFrameEventData GetEvent(const cv::Mat& img, const cv::Rect& game_rect);
};

class ZoraMonumentDetector
{
private:
	tesseract::TessBaseAPI& _tess_api;
	std::array<std::string, 10> _line1_texts;

public:
	ZoraMonumentDetector(tesseract::TessBaseAPI& api)
		: _tess_api(api) {
	}
	~ZoraMonumentDetector() = default;

	bool Init(const char* lang);

	// returns 0 if not at a monument
	uint8_t GetMonumentID(const cv::Mat& img, const cv::Rect& game_rect);
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

	bool IsTravelButtonPresent(const cv::Mat& img, const cv::Rect& game_rect);
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

	EventType GetEvent(const cv::Mat& img, const cv::Rect& game_rect);
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

	bool IsOnAlbumPage(const cv::Mat& img, const cv::Rect& game_rect);
};