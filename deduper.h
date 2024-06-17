#pragma once
#include "common.h"

class EventDeduper
{
public:
	static void Dedup(const std::multimap<uint32_t, SingleFrameEvent>& events, std::vector<MultiFrameEvent>& out_deduped_events);
	static std::string DedupedEventsToYAMLString(std::vector<MultiFrameEvent>& deduped_events);
	static std::string_view GetMsg(EventType t);
	static EventType GetEventType(const std::string_view &s);
};

class EventAssembler
{
public:
	static void Assemble(const std::vector<MultiFrameEvent>& events, std::vector<std::shared_ptr<AssembledEvent>>& out_assembled_events);
	static std::string AssembledEventsToYAMLString(const std::vector<std::shared_ptr<AssembledEvent>>& assembled_events);
};