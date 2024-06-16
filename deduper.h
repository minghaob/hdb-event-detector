#pragma once
#include "common.h"

class RepeatingSingleFrameEventDeduper
{
public:
	static void Dedup(const std::multimap<uint32_t, SingleFrameEvent>& events, std::set<SingleFrameEventWithDuration>& out_deduped_events);
	static std::string_view GetMsg(SingleFrameEventType t);
};