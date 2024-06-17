#include "deduper.h"

namespace __details
{
	struct EventDedupConfig
	{
		EventType t;
		uint32_t minimal_spacing;		// minimal number of frames needed in-between to be considered none-repeating
	};

	constexpr auto dedup_cfgs = std::to_array<EventDedupConfig>({
		{ EventType::Korok,				90 },
		{ EventType::SpiritOrb,			90 },
		{ EventType::TowerActivation,	90 },
		{ EventType::TravelButton,		90 },
		{ EventType::LoadingScreen,		2 },
		{ EventType::BlackScreen,		2 },
		{ EventType::WhiteScreen,		2 },
	});

	static consteval std::array<uint32_t, uint32_t(EventType::Max)> CreateMinimalSpacingArray()
	{
		std::array<uint32_t, uint32_t(EventType::Max)> ret{};
		ret.fill(1);
		for (size_t i = 0; i < dedup_cfgs.size(); i++)
			ret[std::to_underlying(dedup_cfgs[i].t)] = dedup_cfgs[i].minimal_spacing;
		return ret;
	}

	constexpr auto event_msgs = std::to_array<std::pair<EventType, std::string_view>>({
		{ EventType::Korok,				"Korok Seed" },
		{ EventType::SpiritOrb,			"Spirit Orb" },
		{ EventType::TowerActivation,	"Sheikah Tower activated." },
		{ EventType::TravelButton,		"Travel" },
		{ EventType::LoadingScreen,		"Load Screen" },
		{ EventType::BlackScreen,		"Black Screen" },
		{ EventType::WhiteScreen,		"White Screen" },
		{ EventType::AlbumPage,			"Album Page" },
		{ EventType::Talus,				"Talus" },
		{ EventType::Hinox,				"Hinox" },
		{ EventType::Molduga,			"Molduga" },
		{ EventType::ZoraMonument,		"Zora Monument" },
		{ EventType::Dialog,			"Dialog" },
		{ EventType::Load,				"Load" },
		{ EventType::Warp,				"Warp" },
		{ EventType::Shrine,			"Shrine" },
		{ EventType::Memory,			"Memory" },
		{ EventType::DivineBeast,		"Divine Beast" },
	});

	static consteval bool VerifyMsgTable(const std::array<std::string_view, uint32_t(EventType::Max)> &tabel)
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

constexpr std::array<uint32_t, uint32_t(EventType::Max)> minimal_spacing = __details::CreateMinimalSpacingArray();
constexpr std::array<std::string_view, uint32_t(EventType::Max)> event_message = __details::CreateEventMessageArray();

static_assert(__details::VerifyMsgTable(event_message));

void EventDeduper::Dedup(const std::multimap<uint32_t, SingleFrameEvent>& events, std::vector<MultiFrameEvent>& out_deduped_events)
{
	out_deduped_events.clear();

	std::set<MultiFrameEvent> ordered_deduped_events;

	std::array<MultiFrameEvent, uint32_t(EventType::Max)> active_event{};
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
				ordered_deduped_events.emplace(active_event[t]);
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
			ordered_deduped_events.emplace(active_event[i]);
		}
	}

	out_deduped_events.reserve(ordered_deduped_events.size());
	for (const MultiFrameEvent& e : ordered_deduped_events)
		out_deduped_events.push_back(e);
}

std::string EventDeduper::DedupedEventsToYAMLString(std::vector<MultiFrameEvent>& deduped_events)
{
	std::ostringstream os;
	os << "---" << std::endl;
	os << "events:" << std::endl;
	for (const auto& itor : deduped_events)
		os << "  - [[" << itor.evt.frame_number << ", " << itor.evt.frame_number + itor.duration - 1 << "], \"" << EventDeduper::GetMsg(itor.evt.data.type) << "\"]" << std::endl;

	return os.str();
}

std::string_view EventDeduper::GetMsg(EventType t)
{
	return event_message[std::to_underlying(t)];
}

EventType EventDeduper::GetEventType(const std::string_view& s)
{
	for (uint32_t i = 0; i < std::to_underlying(EventType::Max); i++)
	{
		if (s == event_message[i])
			return EventType(i);
	}

	return EventType::None;
}


struct CompareSharedPointersByValue {
	template <typename T>
	bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const {
		return *lhs < *rhs;
	}
};


struct ShrineEvent : public AssembledEvent
{
	uint32_t frame_offset_enter_end;
	uint32_t frame_offset_leave_begin;

	ShrineEvent(uint32_t start_frame, uint32_t enter_end_frame, uint32_t leave_begin_frame, uint32_t leave_end_frame)
		: AssembledEvent(MultiFrameEvent{
				.evt = {
					.frame_number = start_frame,
					.data = {
						.type = EventType::Shrine,
					},
				},
				.duration = leave_end_frame - start_frame + 1,
			})
	{
		frame_offset_enter_end = enter_end_frame - start_frame;
		frame_offset_leave_begin = leave_begin_frame - start_frame;
	}
	virtual uint32_t GetNumSegments() override { return 3; }
	virtual std::string_view GetSegmentName(uint32_t idx) override
	{
		if (idx == 0)
			return "Enter";
		else if (idx == 1)
			return "Main";
		else if (idx == 2)
			return "Leave";
		return "";
	}
	virtual uint32_t GetSegmentEndFrameOffset(uint32_t idx) override
	{
		if (idx == 0)
			return frame_offset_enter_end;
		else if (idx == 1)
			return frame_offset_leave_begin;
		else if (idx == 2)
			return duration - 1;

		return 0;
	}
};

void EventAssembler::Assemble(const std::vector<MultiFrameEvent>& events, std::vector<std::shared_ptr<AssembledEvent>>& out_assembled_events)
{
	out_assembled_events.reserve(events.size());

	std::set<std::shared_ptr<AssembledEvent>, CompareSharedPointersByValue> event_set;
	for (uint32_t i = 0; i < uint32_t(events.size()); i++)
	{
		// assemble load events
		if (i + 2 < uint32_t(events.size())
			&& events[i].evt.data.type == EventType::BlackScreen
			&& events[i + 1].evt.data.type == EventType::LoadingScreen
			&& events[i + 2].evt.data.type == EventType::BlackScreen)
		{
			MultiFrameEvent load_event{
				.evt = {
					.frame_number = events[i].evt.frame_number,
					.data = {
						.type = EventType::Load,
					},
				},
				.duration = events[i + 2].LastFrame() - events[i].evt.frame_number + 1,
			};
			event_set.emplace(std::make_shared<AssembledEvent>(load_event));
			i += 2;			// skip the next 2 events
			continue;
		}

		// otherwise just forward the event to the set
		event_set.emplace(std::make_shared<AssembledEvent>(events[i]));
	}

	// assemble travel events
	for (auto itor = event_set.begin(); itor != event_set.end(); )
	{
		auto itor_next = std::next(itor);
		if ((*itor)->evt.data.type == EventType::TravelButton)
		{
			auto itor_to_load = event_set.end();
			for (auto itor2 = std::next(itor); itor2 != event_set.end(); itor2++)
			{
				if ((*itor2)->evt.frame_number - (*itor)->LastFrame() >= 300)		// peak 10 seconds ahead, usually it should be around 6 seconds
					break;
				if ((*itor2)->evt.data.type == EventType::Load)
				{
					itor_to_load = itor2;
					break;
				}
			}

			if (itor_to_load != event_set.end())
			{
				MultiFrameEvent load_event{
					.evt = {
						.frame_number = (*itor)->evt.frame_number,
						.data = {
							.type = EventType::Warp,
						},
					},
					.duration = (*itor_to_load)->LastFrame() - (*itor)->evt.frame_number + 1,
				};
				itor_next = event_set.emplace(std::make_shared<AssembledEvent>(load_event)).first;		// the old itor_next might be invalidated by the erases below
				event_set.erase(itor);
				event_set.erase(itor_to_load);
			}
		}
		itor = itor_next;
	}

	// assemble shrine events
	for (auto itor = event_set.begin(); itor != event_set.end(); )
	{
		auto itor_next = std::next(itor);
		if ((*itor)->evt.data.type == EventType::SpiritOrb)
		{
			auto TryAssembleShrineEvent = [&event_set, &itor_next](auto itor_orb) -> bool {
				// Load (enter shrine) before SpiritOrb
				// If there's reload inside shrine,that one will be taken instead. Unfortunately there's no way to distinguish between these two cases a.t.m.
				auto itor_enter_load = itor_orb;
				while ((*itor_enter_load)->evt.data.type != EventType::Load && itor_enter_load != event_set.begin())
					itor_enter_load--;
				if ((*itor_enter_load)->evt.data.type != EventType::Load)
					return false;

				// BlackScreen right after Load (skipping enter cutscene)
				auto itor_blackscreen0 = std::next(itor_enter_load);
				if ((*std::next(itor_enter_load))->evt.data.type != EventType::BlackScreen)
					return false;
				// BlackScreen right before SpiritOrb (after the glowing box breaks)
				auto itor_blackscreen1 = std::prev(itor_orb);
				if ((*itor_blackscreen1)->evt.data.type != EventType::BlackScreen)
					return false;
				// These two BlackScreens must be different ones
				if (itor_blackscreen0 == itor_blackscreen1)
					return false;

				// Load (leave shrine) after SpiritOrb
				auto itor_leave_load = itor_orb;
				while ((*itor_leave_load)->evt.data.type != EventType::Load && itor_leave_load != event_set.end())
					itor_leave_load++;
				if ((*itor_leave_load)->evt.data.type != EventType::Load)
					return false;

				MultiFrameEvent shrine_event{
					.evt = {
						.frame_number = (*itor_enter_load)->evt.frame_number,
						.data = {
							.type = EventType::Shrine,
						},
					},
					.duration = (*itor_leave_load)->LastFrame() - (*itor_enter_load)->evt.frame_number + 1,
				};
				// the old itor_next might be invalidated by the erases below
				itor_next = event_set.emplace(std::make_shared<ShrineEvent>((*itor_enter_load)->evt.frame_number, (*itor_enter_load)->LastFrame(), (*itor_leave_load)->evt.frame_number, (*itor_leave_load)->LastFrame())).first;
				event_set.erase(itor_enter_load);
				event_set.erase(itor_blackscreen0);			// erase this before erasing itor_orb
				event_set.erase(itor_blackscreen1);			// erase this before erasing itor_enter_load
				event_set.erase(itor_orb);
				event_set.erase(itor_leave_load);
				return true;
			};

			if (!TryAssembleShrineEvent(itor))
				std::cout << "Cannot assemble Shrine event from SpiritOrb event at frame " << (*itor)->evt.frame_number << std::endl;
		}
		itor = itor_next;
	}

	for (const auto& e : event_set)
		out_assembled_events.push_back(e);
}

std::string EventAssembler::AssembledEventsToYAMLString(const std::vector<std::shared_ptr<AssembledEvent>>& assembled_events)
{
	std::ostringstream os;
	os << "---" << std::endl;
	os << "events:" << std::endl;
	for (const auto& itor : assembled_events)
	{
		os << "  - frame: [" << itor->evt.frame_number << ", " << itor->evt.frame_number + itor->duration - 1 << "]" << std::endl;
		os << "    type: \"" << EventDeduper::GetMsg(itor->evt.data.type) << "\"" << std::endl;
		if (itor->GetNumSegments() > 1)
		{
			os << "    segments: [";
			for (uint32_t i = 0; i < itor->GetNumSegments(); i++)
			{
				if (i > 0)
					os << ", ";
				os << "[" << itor->GetSegmentEndFrameOffset(i) + itor->evt.frame_number << ", \"" << itor->GetSegmentName(i) << "\"]";
			}
			os << ']' << std::endl;
		}
	}
	
	return os.str();
}