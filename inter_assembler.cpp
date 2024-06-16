#include "inter_assembler.h"

namespace __details
{
	struct RepeatingSingleFrameEventDedupConfig
	{
		SingleFrameEventType t;
		uint32_t minimal_spacing;		// minimal number of frames needed in-between to be considered none-repeating
		std::string_view message;		// message for the event
	};

	constexpr auto simple_events_cfgs = std::to_array<RepeatingSingleFrameEventDedupConfig>({
		{ SingleFrameEventType::Korok,				90,	"Korok Seed" },
		{ SingleFrameEventType::SpiritOrb,			90,	"Spirit Orb" },
		{ SingleFrameEventType::TowerActivation,	90,	"Sheikah Tower activated." },
		{ SingleFrameEventType::TravelButton,		90,	"Travel" },
		{ SingleFrameEventType::LoadingScreen,		2,	"Load Screen" },
		{ SingleFrameEventType::BlackScreen,		2,	"Black Screen" },
		{ SingleFrameEventType::WhiteScreen,		2,	"White Screen" },
	});

	static consteval std::array<uint32_t, uint32_t(SingleFrameEventType::Max)> CreateMinimalSpacingArray()
	{
		std::array<uint32_t, uint32_t(SingleFrameEventType::Max)> ret{};
		ret.fill(1);
		for (size_t i = 0; i < simple_events_cfgs.size(); i++)
			ret[std::to_underlying(simple_events_cfgs[i].t)] = simple_events_cfgs[i].minimal_spacing;
		return ret;
	}

	static consteval std::array<std::string_view, uint32_t(SingleFrameEventType::Max)> CreateEventMessageArray()
	{
		std::array<std::string_view, uint32_t(SingleFrameEventType::Max)> ret{};
		for (size_t i = 0; i < simple_events_cfgs.size(); i++)
			ret[std::to_underlying(simple_events_cfgs[i].t)] = simple_events_cfgs[i].message;
		return ret;
	}
}

constexpr std::array<uint32_t, uint32_t(SingleFrameEventType::Max)> minimal_spacing = __details::CreateMinimalSpacingArray();
constexpr std::array<std::string_view, uint32_t(SingleFrameEventType::Max)> event_message = __details::CreateEventMessageArray();

void RepeatingSingleFrameEventDeduper::Dedup(const std::multimap<uint32_t, SingleFrameEvent>& events, std::set<SingleFrameEventWithDuration>& out_deduped_events)
{
	out_deduped_events.clear();

	std::array<SingleFrameEventWithDuration, uint32_t(SingleFrameEventType::Max)> active_event{};
	active_event.fill({.duration = 0});

	for (const auto& e : events)
	{
		auto t = std::to_underlying(e.second.data.type);
		if (active_event[t].duration == 0)
		{
			active_event[t].evt = e.second;
			active_event[t].duration = 1;
		}
		else
		{
			if (active_event[t].evt.frame_number + active_event[t].duration - 1 + minimal_spacing[t] <= e.second.frame_number || active_event[t].evt.data != e.second.data)
			{
				out_deduped_events.emplace(active_event[t]);
				active_event[t].evt = e.second;
				active_event[t].duration = 1;
			}
			else
				active_event[t].duration = e.second.frame_number - active_event[t].evt.frame_number + 1;
		}
	}

	for (size_t i = 0; i < active_event.size(); i++)
	{
		if (active_event[i].duration != 0)
		{
			out_deduped_events.emplace(active_event[i]);
		}
	}
}

std::string_view RepeatingSingleFrameEventDeduper::GetMsg(SingleFrameEventType t)
{
	return event_message[std::to_underlying(t)];
}