#include <filesystem>
#include <thread>

#include "common.h"
#include "location_detector.h"
#include "item_detector.h"
#include "tower_activation.h"
#include "config.h"


struct Event
{
	uint32_t video_idx;
	uint32_t frame_number;
	std::string message;
};

template <typename T>
class ThreadSafeQueue {
private:
	std::mutex mutex;
	std::queue<T> queue;
	std::condition_variable cond;
	std::atomic<bool> done{ false };

public:
	void push(T value) {
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(std::move(value));
		cond.notify_one();
	}

	// Timed pop with 1-second wake-up
	bool pop(T& value, uint32_t wait_ms) {
		std::unique_lock<std::mutex> lock(mutex);
		while (queue.empty() && !done) {
			// Wait for up to 1 second
			if (cond.wait_for(lock, std::chrono::milliseconds(wait_ms)) == std::cv_status::timeout) {
				return false; // No new messages, but wake up every second
			}
		}
		if (!queue.empty()) {
			value = std::move(queue.front());
			queue.pop();
			return true;
		}
		return false;
	}

	void finish() {
		done = true;
		cond.notify_all();
	}
};

void AnalyseVideo(uint32_t video_idx, const std::string &video_file, cv::Rect game_rect, uint32_t frame_start, uint32_t frame_end, ThreadSafeQueue<Event> &queue, uint32_t &feedback_frame_number, std::atomic<uint32_t> &ended_thread)
{
	std::string lang = "eng";

	LocationDetector location_detector;
	if (!location_detector.Init(lang.c_str()))
		return;

	ItemDetector item_detector;
	if (!item_detector.Init(lang.c_str()))
		return;

	TowerActivationDetector tower_detector;
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
			std::cout << "video[" << video_idx << "]: fps != 30" << std::endl;
			exit(-1);
		}
		//std::cout << "Frame rate: " << fps << std::endl;
		double duration_sec = num_frames / fps;
		//std::cout << "Duration: " << duration_sec << " seconds" << std::endl;

		double format = cap.get(cv::CAP_PROP_FORMAT);
		//std::cout << "Format: " << format << std::endl;

		if (frame_start >= num_frames || frame_end >= num_frames || frame_start > frame_end)
		{
			std::cout << "video[" << video_idx << "]: segment [" << frame_start << ", " << frame_end << "] has range issues" << std::endl;
			exit(-1);
		}

		if (frame_start > 0)
			cap.set(cv::CAP_PROP_POS_FRAMES, frame_start - 1);			// VideoCapture.read() reads the next frame

		if (game_rect.x < 0 || game_rect.y < 0 || game_rect.x + game_rect.width >(int)width || game_rect.y + game_rect.height >(int)height)
		{
			std::cout << "video[" << video_idx << "]: game image area outside video frame" << std::endl;
			exit(-1);
		}
		//std::cout << "Game area: (" << game_rect.x << ", " << game_rect.y << ") + (" << game_rect.width << ", " << game_rect.height << ")" << std::endl;

		constexpr double item_box_duration = 3;		// 3 seconds for an item box
		const int32_t item_box_duration_nframe = int32_t(item_box_duration * fps + 0.5);
		std::map<std::string, int32_t> last_item_detected_frame;
		int32_t last_tower_detected_frame = -1;
		for (uint32_t frame_number = frame_start; frame_number <= frame_end; frame_number++)
		{
			feedback_frame_number = frame_number;
			DWORD tbegin = ::timeGetTime();
			cv::Mat frame;
			if (!cap.read(frame))
				break;

			int cur_frame = int(cap.get(cv::CAP_PROP_POS_FRAMES));

			DWORD tend = ::timeGetTime();

			std::string item = item_detector.GetItem(frame(game_rect));
			if (!item.empty())
			{
				auto itor = last_item_detected_frame.find(item);
				if (itor == last_item_detected_frame.end() || itor->second <= cur_frame - item_box_duration_nframe)
				{
					if (itor == last_item_detected_frame.end())
						itor = last_item_detected_frame.emplace(item, cur_frame).first;
					Event evt;
					evt.frame_number = frame_number;
					evt.video_idx = video_idx;
					evt.message = item;
					queue.push(evt);
				}
				itor->second = cur_frame;
			}

			if (tower_detector.IsActivatingTower(frame(game_rect)))
			{
				if (last_tower_detected_frame < 0 || last_tower_detected_frame <= cur_frame - item_box_duration_nframe)
				{
					Event evt;
					evt.frame_number = frame_number;
					evt.video_idx = video_idx;
					evt.message = "Sheikah Tower activated.";
					queue.push(evt);
				}
				last_tower_detected_frame = cur_frame;
			}
		}
	}
	else
	{
		std::cout << "Cannot open video file " << video_file << std::endl;
		exit(-1);
	}

	ended_thread++;
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

	std::size_t num_segments = 0;
	for (uint32_t i = 0; i < uint32_t(cfg.videos.size()); i++)
		num_segments += cfg.videos[i].segments.size();

	ThreadSafeQueue<Event> event_queue;							// workers push, main thread pops
	std::vector<uint32_t> thread_current_frame(num_segments);	// workers write, main thread read
	std::vector<uint32_t> start_frame(num_segments);
	std::vector<uint32_t> total_frame(num_segments);
	std::atomic<uint32_t> ended_thread = 0;

	std::vector<std::thread> threads;

	for (uint32_t i = 0; i < uint32_t(cfg.videos.size()); i++)
	{
		for (uint32_t j = 0; j < uint32_t(cfg.videos[i].segments.size()); j++)
		{
			start_frame[threads.size()] = cfg.videos[i].segments[j].start_frame;
			total_frame[threads.size()] = cfg.videos[i].segments[j].end_frame - cfg.videos[i].segments[j].start_frame + 1;
			thread_current_frame[threads.size()] = start_frame[threads.size()];
			threads.emplace_back(AnalyseVideo,
				i,
				(yaml_path / cfg.videos[i].filename).string(),
				cv::Rect(cfg.videos[i].bbox_left, cfg.videos[i].bbox_top, cfg.videos[i].bbox_right - cfg.videos[i].bbox_left + 1, cfg.videos[i].bbox_bottom - cfg.videos[i].bbox_top + 1),
				cfg.videos[i].segments[j].start_frame,
				cfg.videos[i].segments[j].end_frame,
				std::ref(event_queue),
				std::ref(thread_current_frame[threads.size()]),
				std::ref(ended_thread));
		}
	}

	std::vector<std::multimap<uint32_t, Event>> all_events(cfg.videos.size());		// one for each video
	Event evt;
	std::map<std::string, uint32_t> event_counter;
	while (1)
	{
		if (!event_queue.pop(evt, 30))
		{
			std::ostringstream os;
			for (std::size_t i = 0; i < thread_current_frame.size(); i++)
			{
				char buf[40];
				{
					int frame_in_sec = thread_current_frame[i] % 30;
					int sec = thread_current_frame[i] / 30;
					sprintf_s(buf, "[%d] %02d:%02d:%02d.%02d(%.1lf%%) ", thread_current_frame[i], sec / 3600, sec % 3600 / 60, sec % 60, frame_in_sec, (thread_current_frame[i] - start_frame[i] + 1) * 100.0f / total_frame[i]);
				}
				os << buf;
			}
			std::cout << os.str() << std::string(120 - os.str().size(), ' ') << '\r';
		}
		else
		{
			char buf[100];
			{
				int frame_in_sec = evt.frame_number % 30;
				int sec = evt.frame_number / 30;
				sprintf_s(buf, "v[%d]: [%d] %02d:%02d:%02d.%02d %s", evt.video_idx, evt.frame_number, sec / 3600, sec % 3600 / 60, sec % 60, frame_in_sec, evt.message.c_str());
			}
			std::cout << buf << std::string(120 - strlen(buf), ' ') << std::endl;
			all_events[evt.video_idx].emplace(evt.frame_number, evt);
			if (event_counter.find(evt.message) == event_counter.end())
				event_counter.emplace(evt.message, 1);
			else
				event_counter[evt.message]++;
		}

		if (ended_thread == uint32_t(threads.size()))
		{
			for (std::size_t i = 0; i < threads.size(); i++)
				threads[i].join();
			break;
		}
	}
	std::cout << std::endl;

	if (yaml_file_path.filename() == "run.yaml")
	{
		for (uint32_t i = 0; i < uint32_t(cfg.videos.size()); i++)
		{
			std::ostringstream os;
			os << "---" << std::endl;
			os << "events:" << std::endl;
			for (const auto& itor : all_events[i])
				os << "  - [" << itor.second.frame_number << ", \"" << itor.second.message << "\"]" << std::endl;

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
