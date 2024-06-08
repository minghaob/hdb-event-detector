#include <filesystem>
#include <thread>
#include <ranges>
#include <numeric>

#include "common.h"
#include "location_detector.h"
#include "item_detector.h"
#include "tower_activation.h"
#include "config.h"
#include "scheduler.h"

class TesseractAPI
{
private:
	tesseract::TessBaseAPI _api;

public:
	bool Init(const char* lang)
	{
		if (_api.Init(".", lang))
		{
			std::cout << "OCRTesseract: Could not initialize tesseract." << std::endl;
			return false;
		}

		// use single line mode
		_api.SetPageSegMode(tesseract::PageSegMode::PSM_SINGLE_LINE);

		// limit to these characters
		if (std::string_view(lang) == "eng")
		{
			if (!_api.SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.'- "))
				return false;
		}
		// ignore extra space at the end of the line without any text, doesn't seem to make much difference though
		if (!_api.SetVariable("gapmap_use_ends", "true"))
			return false;

		return true;
	}

	TesseractAPI() = default;
	~TesseractAPI()
	{
		_api.Clear();
	}

	tesseract::TessBaseAPI& API()
	{
		return _api;
	}
};

void AnalyseVideo(const std::string &video_file, cv::Rect game_rect, std::vector<IntraFrameEvent> &outEvents, uint32_t &num_frame_parsed, VideoParserScheduler &scheduler, const ::GROUP_AFFINITY *thread_affinity)
{
	::SetThreadGroupAffinity(::GetCurrentThread(), thread_affinity, nullptr);

	std::string lang = "eng";

	// location detector uses it's own tesseract api instance
	LocationDetector location_detector;
	if (!location_detector.Init(lang.c_str()))
		return;

	TesseractAPI shared_tess_api;
	if (!shared_tess_api.Init(lang.c_str()))
		return;

	ItemDetector item_detector(shared_tess_api.API());
	if (!item_detector.Init(lang.c_str()))
		return;

	TowerActivationDetector tower_detector(shared_tess_api.API());
	if (!tower_detector.Init(lang.c_str()))
		return;

	cv::VideoCapture cap(video_file);
	if (cap.isOpened())
	{
		double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
		double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
		//std::cout << "File: " << video_file << std::endl;
		//std::cout << "Frame size: " << width << "x" << height << std::endl;
		double num_frames = uint32_t(cap.get(cv::CAP_PROP_FRAME_COUNT));
		//std::cout << "Number of frames: " << (int)num_frames << std::endl;

		// Get the frame rate of the video
		double fps = cap.get(cv::CAP_PROP_FPS);
		if (fps != 30)
		{
			std::cout << video_file << ": fps != 30" << std::endl;
			exit(-1);
		}
		//std::cout << "Frame rate: " << fps << std::endl;
		double duration_sec = num_frames / fps;
		//std::cout << "Duration: " << duration_sec << " seconds" << std::endl;

		double format = cap.get(cv::CAP_PROP_FORMAT);
		//std::cout << "Format: " << format << std::endl;

		int work_item = -1;
		uint32_t frame_start = 0, frame_end = 0;

		while (true)
		{
			uint32_t last_frame_end = frame_end;
			work_item = scheduler.GetNextWorkItem(work_item, frame_start, frame_end);
			if (work_item < 0)
				break;

			if (frame_start >= num_frames || frame_end >= num_frames || frame_start > frame_end)
			{
				std::cout << "segment [" << frame_start << ", " << frame_end << "] has range issues" << std::endl;
				exit(-1);
			}

			if (frame_start != uint32_t(cap.get(cv::CAP_PROP_POS_FRAMES)))
				cap.set(cv::CAP_PROP_POS_FRAMES, frame_start);

			if (game_rect.x < 0 || game_rect.y < 0 || game_rect.x + game_rect.width >(int)width || game_rect.y + game_rect.height >(int)height)
			{
				std::cout << "game image area outside video frame" << std::endl;
				exit(-1);
			}

			for (uint32_t cur_frame = frame_start; cur_frame <= frame_end; cur_frame++)
			{
				cv::Mat frame;
				if (!cap.read(frame))
					break;

				std::string item = item_detector.GetItem(frame(game_rect));
				if (!item.empty())
				{
					if (item == "Korok Seed")
					{
						outEvents.push_back({
							.frame_number = cur_frame,
							.type = IntraFrameEventType::Korok,
						});
					}
					else if (item == "Spirit Orb")
					{
						outEvents.push_back({
							.frame_number = cur_frame,
							.type = IntraFrameEventType::SpiritOrb,
						});
					}
				}

				if (tower_detector.IsActivatingTower(frame(game_rect)))
				{
					outEvents.push_back({
							.frame_number = cur_frame,
							.type = IntraFrameEventType::TowerActivation,
					});
				}

				num_frame_parsed++;
			}
		}
	}
	else
	{
		std::cout << "Cannot open video file " << video_file << std::endl;
		exit(-1);
	}
}

int main(int argc, char* argv[])
{
	// just in case of non-ansi text in console
	::SetConsoleOutputCP(CP_UTF8);

	// for shorter thread time slices
	::timeBeginPeriod(1);

	// disable sleep mode
	::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

	if (argc < 2)
		return 0;

	namespace fs = std::filesystem;
	fs::path yaml_path = argv[1];
	fs::path yaml_file_path;
	if (fs::is_regular_file(yaml_path))
	{
		yaml_file_path = yaml_path;
		yaml_path = yaml_path.parent_path();
	}
	else if (fs::is_directory(yaml_path))
		yaml_file_path = yaml_path / "run.yaml";
	else
	{
		std::cout << "Invalid path or file name: " << yaml_path.string() << std::endl;
		return 0;
	}

	RunConfig cfg;
	if (!LoadRunYaml(cfg, yaml_file_path.string()))
		return 0;

	for (uint32_t i = 0; i < uint32_t(cfg.videos.size()); i++)
	{
		if (!std::filesystem::exists(yaml_path / cfg.videos[i].filename))
		{
			std::cout << "videos[" << i << "] '" << cfg.videos[i].filename << "' does not exist";
			return 0;
		}
	}

	VideoParserScheduler scheduler;
	uint32_t num_threads = scheduler.GetNumThreads();
	if (auto itor = cfg.options.find("num_threads"); itor != cfg.options.end())
	{
		try {
			int32_t overriden_num_threads = std::stoi(itor->second);
			if (overriden_num_threads < 1)
			{
				std::cout << "Invalid num_threads value '" << itor->second << "' in " << yaml_path.string() << std::endl;
				return 0;
			}
			if (uint32_t(overriden_num_threads) > num_threads)
			{
				overriden_num_threads = num_threads;
				std::cout << "num_threads value '" << itor->second << "' in " << yaml_path.string() << " to large. Clamping to number of CPU cores - 1." << std::endl;
			}
			num_threads = uint32_t(overriden_num_threads);
		}
		catch (...)
		{
			std::cout << "Invalid num_threads value '" << itor->second << "' in " << yaml_path.string() << std::endl;
			return 0;
		}
	}
	std::cout << "Processing with " << num_threads << " work threads" << std::endl;

	std::map<std::string, uint32_t> event_counter;

	for (uint32_t i = 0; i < uint32_t(cfg.videos.size()); i++)
	{
		std::multimap<uint32_t, IntraFrameEvent> merged_events;

		for (uint32_t j = 0; j < uint32_t(cfg.videos[i].segments.size()); j++)
		{
			DWORD tbegin = ::timeGetTime();
			scheduler.AllocateWorkBatch(cfg.videos[i].segments[j].start_frame, cfg.videos[i].segments[j].end_frame);
			std::vector<std::thread> threads;
			std::atomic<uint32_t> num_ended_thread = 0;
			std::vector<std::vector<IntraFrameEvent>> events(num_threads);
			std::vector<uint32_t> num_frame_parsed(num_threads, 0);
			for (uint32_t thd_idx = 0; thd_idx < num_threads; thd_idx++)
			{
				threads.emplace_back(AnalyseVideo,
					(yaml_path / cfg.videos[i].filename).string(),
					cv::Rect(cfg.videos[i].bbox_left, cfg.videos[i].bbox_top, cfg.videos[i].bbox_right - cfg.videos[i].bbox_left + 1, cfg.videos[i].bbox_bottom - cfg.videos[i].bbox_top + 1),
					std::ref(events[thd_idx]),
					std::ref(num_frame_parsed[thd_idx]),
					std::ref(scheduler),
					scheduler.GetThreadAffinity(thd_idx));
			}
			uint32_t num_frame_total = cfg.videos[i].segments[j].end_frame - cfg.videos[i].segments[j].start_frame + 1;
			DWORD fps_tbegin = ::timeGetTime();
			uint32_t fps = 0;
			uint32_t last_frame_parsed = 0;
			while (1)
			{
				uint32_t total_frame_parsed = std::accumulate(num_frame_parsed.begin(), num_frame_parsed.end(), 0);

				DWORD fps_tend = ::timeGetTime();
				if (fps_tend - fps_tbegin > 200)
				{
					fps = uint32_t((total_frame_parsed - last_frame_parsed) * 1000.0 / (fps_tend - fps_tbegin));
					last_frame_parsed = total_frame_parsed;
					fps_tbegin = fps_tend;
				}

				if (total_frame_parsed == num_frame_total)
				{
					uint32_t process_time_in_sec = (::timeGetTime() - tbegin) / 1000;
					std::string str = "video[" + std::to_string(i) + "].segment[" + std::to_string(j) + "]: " + util::FrameToTimeString(num_frame_total) + " done. (Processed in " + util::SecondToTimeString(process_time_in_sec) + ")";
					std::cout << '\r' << str << std::string(100 - str.size(), ' ') << std::string(100 - str.size(), '\b');
					break;
				}

				std::string str = "video[" + std::to_string(i) + "].segment[" + std::to_string(j) + "]: " + util::FrameToTimeString(total_frame_parsed) + "/" + util::FrameToTimeString(num_frame_total) + " done. (Processing at " + std::to_string(fps) + " fps)";
				std::cout << '\r' << str << std::string(100 - str.size(), ' ') << std::string(100 - str.size(), '\b');

				std::this_thread::sleep_for(std::chrono::milliseconds(30));
			}
			for (uint32_t thd_idx = 0; thd_idx < uint32_t(threads.size()); thd_idx++)
				threads[thd_idx].join();
			std::cout << std::endl;

			for (const auto &thd_events : events)
				for (const auto &event : thd_events)
					merged_events.emplace(event.frame_number, event);
		}

		int32_t last_korok_frame = -1;
		int32_t last_spirit_orb_frame = -1;
		int32_t last_tower_activation_frame = -1;
		std::multimap<uint32_t, std::string > all_events;
		for (const auto& event : merged_events)
		{
			if (event.second.type == IntraFrameEventType::Korok)
			{
				if (last_korok_frame < 0 || uint32_t(last_korok_frame + 90) <= event.second.frame_number)
				{
					//printf("[%d] Korok Seed\n", event.second.frame_number);
					all_events.emplace(event.second.frame_number, "Korok Seed");
					event_counter.try_emplace("Korok Seed", 0).first->second++;
				}
				last_korok_frame = event.second.frame_number;
			}
			if (event.second.type == IntraFrameEventType::SpiritOrb)
			{
				if (last_spirit_orb_frame < 0 || uint32_t(last_spirit_orb_frame + 90) <= event.second.frame_number)
				{
					//printf("[%d] Spirit Orb\n", event.second.frame_number);
					all_events.emplace(event.second.frame_number, "Spirit Orb");
					event_counter.try_emplace("Spirit Orb", 0).first->second++;
				}
				last_spirit_orb_frame = event.second.frame_number;
			}
			if (event.second.type == IntraFrameEventType::TowerActivation)
			{
				if (last_tower_activation_frame < 0 || uint32_t(last_tower_activation_frame + 90) <= event.second.frame_number)
				{
					//printf("[%d] Sheikah Tower activated.\n", event.second.frame_number);
					all_events.emplace(event.second.frame_number, "Sheikah Tower activated.");
					event_counter.try_emplace("Sheikah Tower activated.", 0).first->second++;
				}
				last_tower_activation_frame = event.second.frame_number;
			}
		}

		if (yaml_file_path.filename() == "run.yaml")
		{
			std::ostringstream os;
			os << "---" << std::endl;
			os << "events:" << std::endl;
			for (const auto& itor : all_events)
				os << "  - [" << itor.first << ", \"" << itor.second << "\"]" << std::endl;

			fs::path raw_path = yaml_path / ("raw_" + (i < 9 ? "0" + std::to_string(i + 1) : std::to_string(i + 1)) + ".yaml");		// raw files start at 01
			std::ofstream ofs(raw_path.string());
			if (!ofs.is_open())
				std::cout << os.str();
			else
				ofs << os.str();
		}
	}

	for (auto& itor : event_counter)
		std::cout << itor.first << ": " << itor.second << std::endl;

	return 0;
}
