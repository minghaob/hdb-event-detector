#pragma once
#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>
#include <tesseract/publictypes.h>

#pragma warning(disable:4819)
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

//tesseract
#ifdef _DEBUG
#pragma comment(lib, "archive.lib")
#pragma comment(lib, "bz2d.lib")
#pragma comment(lib, "gif.lib")
#pragma comment(lib, "jpeg.lib")
#pragma comment(lib, "leptonica-1.84.1d.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libcurl-d.lib")
#pragma comment(lib, "libpng16d.lib")
#pragma comment(lib, "lz4d.lib")
#pragma comment(lib, "lzma.lib")
#pragma comment(lib, "tesseract53d.lib")
#pragma comment(lib, "tiffd.lib")
//#pragma comment(lib, "webpd.lib")
//#pragma comment(lib, "webpdecoderd.lib")
#pragma comment(lib, "zlibd.lib")
#pragma comment(lib, "zstd.lib")
#else
#pragma comment(lib, "archive.lib")
#pragma comment(lib, "bz2.lib")
#pragma comment(lib, "gif.lib")
#pragma comment(lib, "jpeg.lib")
#pragma comment(lib, "leptonica-1.84.1.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "libpng16.lib")
#pragma comment(lib, "lz4.lib")
#pragma comment(lib, "lzma.lib")
#pragma comment(lib, "tesseract53.lib")
#pragma comment(lib, "tiff.lib")
//#pragma comment(lib, "webp.lib")
//#pragma comment(lib, "webpdecoder.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "zstd.lib")
#endif
#pragma comment(lib, "CRYPT32.LIB")

//opencv
#ifdef _DEBUG
#pragma comment(lib, "opencv_world4d.lib")
#else
#pragma comment(lib, "opencv_world4.lib")
#endif

#pragma comment(lib, "winmm.lib")

namespace util
{
	/**
	 * Get the edit distance between two strings.
	 * if the distance is larger than max_allowed_edits, returns max_allowed_edits + 1
	 */
	uint32_t GetStringEditDistance(const std::string& first, const std::string& second, uint32_t max_allowed_edits);


	/**
	 * Reorder channels of an opencv Mat in BGRA format to Leptonica RGBA order
	 */
	void OpenCvMatBGRAToLeptonicaRGBAInplace(cv::Mat& frame);

	std::string FrameToTimeString(uint32_t frame);
}

enum class IntraFrameEventType : uint8_t
{
	Korok,
	SpiritOrb,
	TowerActivation,
	LoadingScreen,
	AlbumPage,
	Talus,
	Hinox,
	Molduga,
	ZoraMonument,
	Dialog,
};

struct IntraFrameEvent
{
	uint32_t frame_number;
	IntraFrameEventType type;
	union {
		struct {
			uint8_t num_shrines;
			uint8_t num_koroks;
		} loadscreen_data;
		struct {
			uint8_t talus_id;
		} talus_data;
		struct {
			uint8_t hinox_id;
		} hinox_data;
		struct {
			uint8_t molduga_id;
		} molduga_data;
		struct {
			uint8_t monument_id;
		} monument_data;
		struct {
			uint8_t quest_id;
			uint8_t dialog_id;
			uint8_t npc_id;
		} dialog_data;
	};
};
