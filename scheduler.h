#pragma once
#include <vector>
#include <map>
#include <mutex>
#define NOMINMAX
#include <Windows.h>

class VideoParserScheduler
{
private:
	std::vector<::GROUP_AFFINITY> m_thread_group_affinity;
	uint32_t m_num_work_items;
	std::vector<std::pair<uint32_t, uint32_t>> m_work_item_segments;
	std::mutex m_item_mutex;
	uint32_t m_start_frame, m_end_frame;

private:
	void ItemIndexToStartEndFrame(uint32_t item_index, uint32_t& next_item_frame_start, uint32_t& next_item_frame_end);
public:
	VideoParserScheduler();
	uint32_t AllocateWorkBatch(uint32_t start_frame, uint32_t end_frame);
	uint32_t GetNumThreads() const {
		return uint32_t(m_thread_group_affinity.size());
	}

	// last_item = -1 to indicate there's no last item
	int32_t GetNextWorkItem(int32_t last_item, uint32_t& next_item_frame_start, uint32_t& next_item_frame_end);
	uint32_t GetNumRemainingWorkItems();
	uint32_t GetNumTotalWorkItems();
	const ::GROUP_AFFINITY* GetThreadAffinity(uint32_t thread_idx) const {
		return &m_thread_group_affinity[thread_idx];
	}
};