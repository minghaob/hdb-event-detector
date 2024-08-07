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

enum class EventType : uint8_t
{
	// single frame events
	None,
	SingleFrameEventBegin,
	Korok,
	SpiritOrb,
	TowerActivation,
	TravelButton,
	LoadingScreen,
	BlackScreen,
	WhiteScreen,
	AlbumPage,
	StoneTalus,
	FrostTalus,
	IgneoTalus,
	Stalnox,
	Molduga,
	ZoraMonument,
	Dialog,
	GateRegistered,
	SlateAuthenticated,
	RevaliGale,
	UrbosaFury,
	MiphaGrace,
	DarukProtection,
	Paraglider,
	ThunderHelm,
	SingleFrameEventEnd,

	// assembled events
	AssembledEventBegin,
	Load,			// = BlackScreen + LoadScreen + BlackScreen
	Warp,			// = TravelButton + Load
	Shrine,			// = Load + BlackScreen + BlackScreen + SpiritOrb + Load
	Memory,			// = AlbumPage + WhiteScreen [+ WhiteScreen]
	Medoh,			// = GateRegistered + SlateAuthenticated * 5 + BlackScreen + WhiteScreen + RevaliGale
	Naboris,		// = GateRegistered + SlateAuthenticated * 5 + BlackScreen + WhiteScreen + UrbosaFury
	Ruta,			// = GateRegistered + SlateAuthenticated * 5 + BlackScreen + WhiteScreen + MiphaGrace
	Rudania,		// = GateRegistered + SlateAuthenticated * 5 + BlackScreen + WhiteScreen + DarukProtection
	AssembledEventEnd,

	Max,
};

enum class DialogId : uint8_t
{
	None,
	Kass1,			// blood moon quest
	Kass2,			// two ring quest
	Kass3,			// stag quest
	Kass4,			// master of wind, land on paraglider quest
	Kass5,			// Sasa Kai quest
	Kass6,			// farosh scale quest
	Kass7,			// thunder strike quest
	Kass8,			// 300 gold cache quest
	Vilia,			// sells gerudo outfit
	Straia,			// giant horse quest
	Jini,			// archery challenge quest
	Dugby,			// drillshaft quest
	Zyle,			// gerudo canyon horse quest
	Palme,			// 4 friends of Sesami
	Flaxel,
	Canolo,
	Oliff,
	Sesami1,		// get Sesami quest
	Sesami2,		// turn-in Sesami quest
	Pirou,			// 55 rushroom quest
	Perda,			// nightshade gift quest
	Kheel,			// at warbler's nest
	LeviathanBros,	// Akrah, Garshon and Onya, dialog depends on who is talked to last
	Max,
};

namespace util
{
	/**
	 * Get the edit distance between two strings.
	 * if the distance is larger than max_allowed_edits, returns max_allowed_edits + 1
	 */
	uint32_t GetStringEditDistance(const std::string_view& first, const std::string_view& second, uint32_t max_allowed_edits);

	void UnifyAmbiguousChars(std::string& str);

	/**
	 * Reorder channels of an opencv Mat in BGRA format to Leptonica RGBA order
	 */
	void OpenCvMatBGRAToLeptonicaRGBAInplace(cv::Mat& frame);

	std::string FrameToTimeString(uint32_t frame);
	std::string SecondToTimeString(uint32_t sec);

	std::string_view GetEventText(EventType t);
	EventType GetEventType(const std::string_view& s);
	DialogId GetDialogId(const std::string_view& s);
	std::string_view DialogIdToString(DialogId id);
}

#pragma pack(push, 1)
struct SingleFrameEventData
{
	EventType type;
	union {
		struct {
			uint8_t num_shrines;
			uint16_t num_koroks;
		} loadscreen_data;
		struct {
			uint8_t monument_id;
		} monument_data;
		struct {
			DialogId dialog_id;
		} dialog_data;
	};
	bool operator==(const SingleFrameEventData& other) const
	{
		if (type != other.type)
			return false;
		switch (type)
		{
		case EventType::ZoraMonument:
			return monument_data.monument_id == other.monument_data.monument_id;
		case EventType::Dialog:
			return dialog_data.dialog_id == other.dialog_data.dialog_id;
			break;
		default:
			return true;
		}
	}
};
#pragma pack(pop)

struct SingleFrameEvent
{
	uint32_t frame_number;
	SingleFrameEventData data;
};

struct MultiFrameEvent
{
	SingleFrameEvent evt;
	uint32_t duration;

	bool operator<(const MultiFrameEvent& other) const
	{
		return std::tie(evt.frame_number, duration, evt.data.type) < std::tie(other.evt.frame_number, other.duration, other.evt.data.type);
	}

	uint32_t LastFrame() const
	{
		return evt.frame_number + duration - 1;
	}
};

struct AssembledEvent : public MultiFrameEvent
{
	AssembledEvent(const MultiFrameEvent &e)
		: MultiFrameEvent(e)
	{}
	virtual uint32_t GetNumSegments() { return 1; }
	virtual std::string_view GetSegmentName(uint32_t idx) { return ""; }
	virtual uint32_t GetSegmentEndFrameOffset(uint32_t idx) { return 0; }
};