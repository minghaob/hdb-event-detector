#include "common.h"

namespace __details{
	constexpr auto event_msgs = std::to_array<std::pair<EventType, std::string_view>>({
		{ EventType::Korok,				"Korok Seed" },
		{ EventType::SpiritOrb,			"Spirit Orb" },
		{ EventType::TowerActivation,	"Tower Activation" },
		{ EventType::TravelButton,		"Travel Button" },
		{ EventType::LoadingScreen,		"Loading Screen" },
		{ EventType::BlackScreen,		"Black Screen" },
		{ EventType::WhiteScreen,		"White Screen" },
		{ EventType::AlbumPage,			"Album Page" },
		{ EventType::StoneTalus,		"Stone Talus" },
		{ EventType::FrostTalus,		"Frost Talus" },
		{ EventType::IgneoTalus,		"Igneo Talus" },
		{ EventType::Stalnox,			"Stalnox" },
		{ EventType::Molduga,			"Molduga" },
		{ EventType::ZoraMonument,		"Zora Monument" },
		{ EventType::Dialog,			"Dialog" },
		{ EventType::Load,				"Load" },
		{ EventType::Warp,				"Warp" },
		{ EventType::Shrine,			"Shrine" },
		{ EventType::Memory,			"Memory" },
		{ EventType::DivineBeast,		"Divine Beast" },
		});

	static consteval bool VerifyMsgTable(const std::array<std::string_view, uint32_t(EventType::Max)>& tabel)
	{
		for (auto i = std::to_underlying(EventType::SingleFrameEventBegin) + 1; i < std::to_underlying(EventType::SingleFrameEventEnd); i++)
			if (tabel[i].size() == 0)
				return false;
		for (auto i = std::to_underlying(EventType::AssembledEventBegin) + 1; i < std::to_underlying(EventType::AssembledEventEnd); i++)
			if (tabel[i].size() == 0)
				return false;
		return true;
	}
	static consteval std::array<std::string_view, uint32_t(EventType::Max)> CreateEventMessageArray()
	{
		std::array<std::string_view, uint32_t(EventType::Max)> ret{};
		for (size_t i = 0; i < event_msgs.size(); i++)
			ret[std::to_underlying(event_msgs[i].first)] = event_msgs[i].second;

		return ret;
	}
}

constexpr std::array<std::string_view, uint32_t(EventType::Max)> event_message = __details::CreateEventMessageArray();
static_assert(__details::VerifyMsgTable(event_message));

namespace util
{

uint32_t GetStringEditDistance(const std::string& first, const std::string& second, uint32_t max_allowed_edits)
{
	uint32_t m = uint32_t(first.length());
	uint32_t n = uint32_t(second.length());
	uint32_t s = max_allowed_edits;

	std::vector<uint32_t> T((m + 1) * (n + 1));		// TODO: make this thread_local to avoid multiple memory allocations, or just use alloca()
	T[0] = 0;
	for (uint32_t i = 1; i <= std::min(s, n); i++) {
		T[i * (n + 1) + 0] = i;
	}

	for (uint32_t j = 1; j <= std::min(s, m); j++) {
		T[0 * (n + 1) + j] = j;
	}

	for (uint32_t i = 1; i <= m; i++) {
		uint32_t startColumn = std::max(i, s) - s + 1;
		if (startColumn >= (n + 1))
			return max_allowed_edits + 1;
		uint32_t endColumn = std::min(i + s, n);
		uint32_t minInRow = T[i * (n + 1)];
		if (i > s)
			T[i * (n + 1) + startColumn - 1] = max_allowed_edits;
		if (i + s <= n)
			T[(i - 1) * (n + 1) + endColumn] = max_allowed_edits;
		for (uint32_t j = startColumn; j <= endColumn; j++) {
			uint32_t weight = first[i - 1] == second[j - 1] ? 0 : 1;
			T[i * (n + 1) + j] = std::min(std::min(T[(i - 1) * (n + 1) + j] + 1, T[i * (n + 1) + j - 1] + 1), T[(i - 1) * (n + 1) + j - 1] + weight);
			minInRow = std::min(minInRow, T[i * (n + 1) + j]);
		}
		if (minInRow > max_allowed_edits)
			return max_allowed_edits + 1;
	}

	return T[m * (n + 1) + n];
}

void OpenCvMatBGRAToLeptonicaRGBAInplace(cv::Mat& frame)
{
	//                 byte[0] byte[1] byte[2] byte[3]
	// opencv BGRA        B       G       R       A		(little-endian)
	// leptonica RGBA     A       B       G       R     (big-endian)
	for (int row = 0; row < frame.rows; row++)
	{
		uint32_t* data = (uint32_t*)frame.row(row).data;
		for (int col = 0; col < frame.cols; col++)
			data[col] = (data[col] << 8) | (data[col] >> 24);
	}
}

std::string FrameToTimeString(uint32_t frame)
{
	char buf[40];
	int frame_in_sec = frame % 30;
	int sec = frame / 30;
	sprintf_s(buf, "%02d:%02d:%02d.%02d", sec / 3600, sec % 3600 / 60, sec % 60, frame_in_sec);

	return buf;
}

std::string SecondToTimeString(uint32_t sec)
{
	char buf[40];
	sprintf_s(buf, "%02d:%02d:%02d", sec / 3600, sec % 3600 / 60, sec % 60);

	return buf;
}

std::string_view GetEventText(EventType t)
{
	return event_message[std::to_underlying(t)];
}

EventType GetEventType(const std::string_view& s)
{
	for (uint32_t i = 0; i < std::to_underlying(EventType::Max); i++)
	{
		if (s == event_message[i])
			return EventType(i);
	}

	return EventType::None;
}

}

static_assert(sizeof(SingleFrameEvent) == 8);