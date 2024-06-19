#pragma once
#include <vector>
#include <string>
#include "common.h"

struct RunConfig
{
	struct Video {
		struct Segment {
			uint32_t start_frame;
			uint32_t end_frame;
		};
		std::string filename;
		uint32_t bbox_left;
		uint32_t bbox_right;
		uint32_t bbox_top;
		uint32_t bbox_bottom;
		std::vector<Segment> segments;
		std::vector<SingleFrameEvent> patches;
	};

	std::string uid;
	std::vector<Video> videos;
	std::map<std::string, std::string> options;
};

bool LoadRunYaml(RunConfig& run_cfg, std::string run_file);
