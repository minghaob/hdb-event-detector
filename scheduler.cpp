#include <iostream>
#include <algorithm>
#include "scheduler.h"

constexpr uint32_t WORK_ITEM_LENGTH_IN_FRAME = 60 * 30;

VideoParserScheduler::VideoParserScheduler()
	: m_start_frame(0)
	, m_end_frame(0)
	, m_num_work_items(0)
{
	std::vector<uint8_t> buffer;
	DWORD buffer_size = 0;
	::GetLogicalProcessorInformationEx(::RelationProcessorCore, nullptr, &buffer_size);
	buffer.resize(buffer_size);
	
	if (::GetLogicalProcessorInformationEx(::RelationProcessorCore, (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)&buffer[0], &buffer_size))
	{
		uint32_t offset = 0;
		while (offset < buffer_size)
		{
			SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* ptr = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)&buffer[offset];
			m_thread_group_affinity.push_back(ptr->Processor.GroupMask[0]);
			offset += ptr->Size;
		}

		// leave 1 core free unless system has only one core
		if (m_thread_group_affinity.size() > 0)
			m_thread_group_affinity.erase(m_thread_group_affinity.begin());
	}
}

uint32_t VideoParserScheduler::AllocateWorkBatch(uint32_t start_frame, uint32_t end_frame)
{
	std::unique_lock<std::mutex> m_item_mutex;

	uint32_t num_frame = end_frame - start_frame + 1;
	m_num_work_items = (num_frame + WORK_ITEM_LENGTH_IN_FRAME - 1) / WORK_ITEM_LENGTH_IN_FRAME;
	m_work_item_segments.clear();
	m_work_item_segments.emplace_back(0, m_num_work_items - 1);
	m_start_frame = start_frame;
	m_end_frame = end_frame;

	return m_num_work_items;
}

void VideoParserScheduler::ItemIndexToStartEndFrame(uint32_t item_index, uint32_t& next_item_frame_start, uint32_t& next_item_frame_end)
{
	next_item_frame_start = m_start_frame + item_index * WORK_ITEM_LENGTH_IN_FRAME;
	next_item_frame_end = std::min(m_start_frame + (item_index + 1) * WORK_ITEM_LENGTH_IN_FRAME - 1, m_end_frame);
}

int32_t VideoParserScheduler::GetNextWorkItem(int32_t last_item, uint32_t &next_item_frame_start, uint32_t &next_item_frame_end)
{
	std::unique_lock<std::mutex> m_item_mutex;

	// no work left
	if (!m_work_item_segments.size())
		return -1;

	// first work item to assign
	if (m_work_item_segments.size() == 1 && m_work_item_segments[0].first == 0)
	{
		ItemIndexToStartEndFrame(0, next_item_frame_start, next_item_frame_end);
		if (m_work_item_segments[0].first >= m_work_item_segments[0].second)
			m_work_item_segments.erase(m_work_item_segments.begin());
		else
			m_work_item_segments[0].first++;

		return 0;
	}

	// assign the work item next to last_item if possible
	if (last_item >= 0)
	{
		for (size_t i = 0; i < m_work_item_segments.size(); i++)
			if (m_work_item_segments[i].first == last_item + 1)
			{
				ItemIndexToStartEndFrame(m_work_item_segments[i].first, next_item_frame_start, next_item_frame_end);
				if (m_work_item_segments[i].first >= m_work_item_segments[i].second)
					m_work_item_segments.erase(m_work_item_segments.begin() + i);
				else
					m_work_item_segments[i].first++;

				return last_item + 1;
			}
	}

	// find the longest segment
	uint32_t longest_seg_idx = 0;
	for (size_t i = 1; i < m_work_item_segments.size(); i++)
		if (m_work_item_segments[i].second - m_work_item_segments[i].first + 1 > m_work_item_segments[longest_seg_idx].second - m_work_item_segments[longest_seg_idx].first + 1)
			longest_seg_idx = uint32_t(i);

	// split the longest segment from middle
	uint32_t item_index = (m_work_item_segments[longest_seg_idx].first + m_work_item_segments[longest_seg_idx].second) / 2;
	ItemIndexToStartEndFrame(item_index, next_item_frame_start, next_item_frame_end);
	if (item_index + 1 <= m_work_item_segments[longest_seg_idx].second)			// push second half to the end
		m_work_item_segments.emplace_back(item_index + 1, m_work_item_segments[longest_seg_idx].second);
	if (m_work_item_segments[longest_seg_idx].first > item_index - 1)				// modify first half in-place or erase
		m_work_item_segments.erase(m_work_item_segments.begin() + longest_seg_idx);
	else
		m_work_item_segments[longest_seg_idx].second = item_index - 1;

	return item_index;

}

uint32_t VideoParserScheduler::GetNumTotalWorkItems()
{
	std::unique_lock<std::mutex> m_item_mutex;
	return m_num_work_items;
}

uint32_t VideoParserScheduler::GetNumRemainingWorkItems()
{
	std::unique_lock<std::mutex> m_item_mutex;
	uint32_t ret = 0;
	for (size_t i = 0; i < m_work_item_segments.size(); i++)
		ret += m_work_item_segments[i].second - m_work_item_segments[i].first + 1;
	return ret;
}
