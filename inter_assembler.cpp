#include "inter_assembler.h"

namespace __details
{
	struct SimpleIntraToInterFrameEventConfig
	{
		IntraFrameEventType t;
		uint32_t minimal_spacing;		// minimal number of frames needed in-between intra-frame events of same type for them to be considered a separate inter-frame events
		std::string_view message;		// message for the event
	};

	constexpr auto simple_events_cfgs = std::to_array<SimpleIntraToInterFrameEventConfig>({
		{ IntraFrameEventType::Korok,			90,	"Korok Seed" },
		{ IntraFrameEventType::SpiritOrb,		90,	"Spirit Orb" },
		{ IntraFrameEventType::TowerActivation,	90,	"Sheikah Tower activated." },
		{ IntraFrameEventType::TravelButton,	90,	"Travel" },
		{ IntraFrameEventType::LoadingScreen,	2,	"Load Screen" },
		{ IntraFrameEventType::BlackScreen,		2,	"Black Screen" },
		{ IntraFrameEventType::WhiteScreen,		2,	"White Screen" },
	});

	static consteval std::array<uint32_t, uint32_t(IntraFrameEventType::Max)> CreateMinimalSpacingArray()
	{
		std::array<uint32_t, uint32_t(IntraFrameEventType::Max)> ret{};
		ret.fill(1);
		for (size_t i = 0; i < simple_events_cfgs.size(); i++)
			ret[std::to_underlying(simple_events_cfgs[i].t)] = simple_events_cfgs[i].minimal_spacing;
		return ret;
	}

	static consteval std::array<std::string_view, uint32_t(IntraFrameEventType::Max)> CreateEventMessageArray()
	{
		std::array<std::string_view, uint32_t(IntraFrameEventType::Max)> ret{};
		for (size_t i = 0; i < simple_events_cfgs.size(); i++)
			ret[std::to_underlying(simple_events_cfgs[i].t)] = simple_events_cfgs[i].message;
		return ret;
	}
}

constexpr std::array<uint32_t, uint32_t(IntraFrameEventType::Max)> minimal_spacing = __details::CreateMinimalSpacingArray();
constexpr std::array<std::string_view, uint32_t(IntraFrameEventType::Max)> event_message = __details::CreateEventMessageArray();

SimpleInterFrameEventAssembler::SimpleInterFrameEventAssembler()
{
	_last_frame.fill(-1);
}

InterFrameEvent SimpleInterFrameEventAssembler::OnNextIntraFrameEvent(const IntraFrameEvent& e)
{
	auto t = std::to_underlying(e.type);
	if (event_message[t].size())
	{
		int32_t old_last_frame = _last_frame[t];
		_last_frame[t] = e.frame_number;
		if (old_last_frame == -1 || uint32_t(old_last_frame) + minimal_spacing[t] <= e.frame_number)
			return { .frame_number = e.frame_number, .message = event_message[t] };
	}

	return {};
}