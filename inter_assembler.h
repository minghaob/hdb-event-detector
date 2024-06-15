#pragma once
#include "common.h"

class SimpleInterFrameEventAssembler
{
	std::array<int32_t, uint32_t(IntraFrameEventType::Max)> _last_frame;

public:
	SimpleInterFrameEventAssembler();
	
	InterFrameEvent OnNextIntraFrameEvent(const IntraFrameEvent& e);
	InterFrameEvent OnIntraFrameEventStreamEnd(const IntraFrameEvent& e);
};