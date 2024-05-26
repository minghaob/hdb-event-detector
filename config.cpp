#include <filesystem>
#include <iostream>
#include "yaml-cpp/yaml.h"
#include "config.h"

bool LoadRunYaml(RunConfig &run_cfg, std::string run_file)
{
	if (!std::filesystem::exists(run_file))
	{
		std::cout << run_file << " : not found" << std::endl;
		return false;
	}
	YAML::Node run_node = YAML::LoadFile(run_file);
	if (!run_node.IsMap())
	{
		std::cout << "run file: root node not a mapping" << std::endl;
		return false;
	}

	YAML::Node uid_node = run_node["uid"];
	if (!uid_node)
	{
		std::cout << "run file: root node does not have 'uid' member" << std::endl;
		return false;
	}
	run_cfg.uid = run_node["uid"].as<std::string>();

	YAML::Node videos_node = run_node["videos"];
	if (!videos_node || !videos_node.IsSequence() || videos_node.size() == 0)
	{
		std::cout << "run file: root node does not have a non-empty sequence member 'videos'." << std::endl;
		return false;
	}
	run_cfg.videos.resize(videos_node.size());

	for (std::size_t video_idx = 0; video_idx < videos_node.size(); video_idx++)
	{
		if (!videos_node[video_idx].IsMap())
		{
			std::cout << "run file: videos[" << video_idx << "] not a mapping" << std::endl;
			return false;
		}

		// .local
		YAML::Node local_node = videos_node[video_idx]["local"];
		if (!local_node)
		{
			std::cout << "run file: videos[" << video_idx << "].local not found" << std::endl;
			return false;
		}
		run_cfg.videos[video_idx].filename = local_node.as<std::string>();

		// .game_rect
		YAML::Node game_rect_node = videos_node[video_idx]["game_rect"];
		if (!game_rect_node || !game_rect_node.IsSequence() || game_rect_node.size() != 4)
		{
			std::cout << "run file: videos[" << video_idx << "].game_rect.size() != 4" << std::endl;
			return false;
		}
		run_cfg.videos[video_idx].bbox_left = game_rect_node[0].as<uint32_t>();
		run_cfg.videos[video_idx].bbox_top = game_rect_node[1].as<uint32_t>();
		run_cfg.videos[video_idx].bbox_right = game_rect_node[2].as<uint32_t>();
		run_cfg.videos[video_idx].bbox_bottom = game_rect_node[3].as<uint32_t>();

		// .segments
		YAML::Node segments_node = videos_node[video_idx]["segments"];
		if (!segments_node || !segments_node.IsSequence() || segments_node.size() == 0)
		{
			std::cout << "run file: videos[" << video_idx << "].segments.size() == 0" << std::endl;
			return false;
		}
		run_cfg.videos[video_idx].segments.resize(segments_node.size());
		for (std::size_t seg_idx = 0; seg_idx < segments_node.size(); seg_idx++)
		{
			if (!segments_node[seg_idx].IsSequence() || segments_node[seg_idx].size() != 2)
			{
				std::cout << "run file: videos[" << video_idx << "].segments[" << seg_idx << "].size() != 2" << std::endl;
				return false;
			}
			run_cfg.videos[video_idx].segments[seg_idx].start_frame = segments_node[seg_idx][0].as<uint32_t>();
			run_cfg.videos[video_idx].segments[seg_idx].end_frame = segments_node[seg_idx][1].as<uint32_t>();
		}
	}

	YAML::Node options_node = run_node["detector_options"];
	if (options_node && options_node.IsMap())
	{
		for (const auto& itor : options_node)
			run_cfg.options.emplace(itor.first.as<std::string>(), itor.second.as<std::string>());
	}

	return true;
}