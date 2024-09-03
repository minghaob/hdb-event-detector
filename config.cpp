#include <filesystem>
#include <iostream>
#include "yaml-cpp/yaml.h"
#include "config.h"
#include "common.h"

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
		for (std::size_t comp_idx = 0; comp_idx < video_idx; comp_idx++)
		{
			if (run_cfg.videos[video_idx].filename == run_cfg.videos[comp_idx].filename)
			{
				std::cout << "run file: videos[" << video_idx << "].local == videos[" << comp_idx << "].local" << std::endl;
				return false;
			}
		}

		// .game_rect
		YAML::Node game_rect_node = videos_node[video_idx]["game_rect"];
		if (!game_rect_node || !game_rect_node.IsSequence() || game_rect_node.size() != 4)
		{
			std::cout << "run file: videos[" << video_idx << "].game_rect.size() != 4" << std::endl;
			return false;
		}
		run_cfg.videos[video_idx].bbox_left = game_rect_node[0].as<int32_t>();
		run_cfg.videos[video_idx].bbox_top = game_rect_node[1].as<int32_t>();
		run_cfg.videos[video_idx].bbox_right = game_rect_node[2].as<int32_t>();
		run_cfg.videos[video_idx].bbox_bottom = game_rect_node[3].as<int32_t>();

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

	YAML::Node patch_node = run_node["patch"];
	if (patch_node && patch_node.IsSequence())
	{
		for (std::size_t patch_idx = 0; patch_idx < patch_node.size(); patch_idx++)
		{
			if (!patch_node[patch_idx].IsMap())
			{
				std::cout << "run file: patch[" << patch_idx << "] is not a map" << std::endl;
				return false;
			}
			YAML::Node cur_patch = patch_node[patch_idx];
			YAML::Node video_idx_node = cur_patch["video"];
			YAML::Node frame_number_node = cur_patch["frame"];
			YAML::Node event_type_node = cur_patch["event"];
			if (!video_idx_node)
			{
				std::cout << "run file: patch[" << patch_idx << "].video is missing" << std::endl;
				return false;
			}
			if (!frame_number_node)
			{
				std::cout << "run file: patch[" << patch_idx << "].frame is missing" << std::endl;
				return false;
			}
			if (!event_type_node)
			{
				std::cout << "run file: patch[" << patch_idx << "].event is missing" << std::endl;
				return false;
			}
			uint32_t video_idx = video_idx_node.as<uint32_t>();
			if (video_idx >= uint32_t(run_cfg.videos.size()))
			{
				std::cout << "run file: patch[" << patch_idx << "].video has invalid value " << video_idx << std::endl;
				return false;
			}
			uint32_t frame_number;
			uint32_t end_frame;
			if (frame_number_node.IsScalar())
			{
				frame_number = frame_number_node.as<uint32_t>();
				end_frame = frame_number;
			}
			else if (frame_number_node.IsSequence() && frame_number_node.size() == 2)
			{
				frame_number = frame_number_node[0].as<uint32_t>();
				end_frame = frame_number_node[1].as<uint32_t>();
				if (end_frame < frame_number)
				{
					std::cout << "run file: patch[" << patch_idx << "].frame has end frame smaller than start frame." << std::endl;
					return false;
				}
			}
			else
			{
				std::cout << "run file: patch[" << patch_idx << "].frame is neither a number nor a sequence of two numbers." << std::endl;
				return false;
			}
			{
				bool frame_valid = false;
				std::vector<RunConfig::Video::Segment>& segs = run_cfg.videos[video_idx].segments;
				for (size_t i = 0 ; i < segs.size(); i++)
					if (frame_number >= segs[i].start_frame && frame_number <= segs[i].end_frame)
					{
						if (end_frame != frame_number && end_frame < segs[i].start_frame || frame_number > segs[i].end_frame)
							break;
						frame_valid = true;
						break;
					}
				if (!frame_valid)
				{
					std::cout << "run file: patch[" << patch_idx << "].frame has invalid value " << frame_number << " that is outside segments" << std::endl;
					return false;
				}
			}
			std::string event_text = event_type_node.as<std::string>();
			EventType event_type;
			bool remove = event_text.size() > 0 && event_text[0] == '-';
			if (remove)
				event_type = util::GetEventType(std::string_view(event_text).substr(1));
			else
				event_type = util::GetEventType(event_text);
			if (event_type == EventType::None)
			{
				std::cout << "run file: patch[" << patch_idx << "].event has unrecognized value \"" << event_text << "\"" << std::endl;
				return false;
			}
			SingleFrameEvent evt = {
				.frame_number = frame_number,
				.data = {
					.type = event_type,
				}
			};
			if (event_type == EventType::Dialog)
			{
				YAML::Node dialog_id_node = cur_patch["dialog_id"];
				if (!dialog_id_node)
				{
					std::cout << "run file: patch[" << patch_idx << "].dialog_id expected for \"" << event_text << "\" event" << std::endl;
					return false;
				}
				evt.data.dialog_data.dialog_id = util::GetDialogId(dialog_id_node.as<std::string>());
				if (evt.data.dialog_data.dialog_id == DialogId::None)
				{
					std::cout << "run file: patch[" << patch_idx << "].dialog_id \"" << dialog_id_node.as<std::string>() << "\" not recognized" << std::endl;
					return false;
				}
			}
			run_cfg.videos[video_idx].patches.push_back({
				.evt = evt,
				.end_frame = end_frame,
				.remove = remove,
			});

		}

	}

	return true;
}