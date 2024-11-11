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
		struct Patch {
			SingleFrameEvent evt;
			uint32_t end_frame;
			bool remove;
		};
		std::string filename;
		int32_t bbox_left;
		int32_t bbox_right;
		int32_t bbox_top;
		int32_t bbox_bottom;
		double color_scale;
		double color_shift;
		std::vector<Segment> segments;
		std::vector<Patch> patches;
	};

	std::string uid;
	std::vector<Video> videos;
	std::map<std::string, std::string> options;
};

bool LoadRunYaml(RunConfig& run_cfg, std::string run_file);
