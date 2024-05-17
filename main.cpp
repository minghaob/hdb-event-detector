#include "common.h"
#include "location_detector.h"
#include "item_detector.h"
#include "tower_activation.h"

//cv::Rect gameRect(412, 114, 1920 - 412, 962 - 114);

void AnalyseVideo(const std::string &video_file, cv::Rect game_rect, int frame_start, int frame_end, const std::string &output_file)
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

	std::ofstream ofs;
	if (output_file.size())
	{
		ofs.open(output_file);
		if (!ofs.is_open())
			std::cout << "Cannot open output file " << output_file << ". Result will not be output to file" << std::endl;
	}

	cv::VideoCapture cap(video_file);
	if (cap.isOpened())
	{
		double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
		double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
		std::cout << "File: " << video_file << std::endl;
		std::cout << "Frame size: " << width << "x" << height << std::endl;
		double num_frames = cap.get(cv::CAP_PROP_FRAME_COUNT);
		std::cout << "Number of frames: " << (int)num_frames << std::endl;

		// Get the frame rate of the video
		double fps = cap.get(cv::CAP_PROP_FPS);
		std::cout << "Frame rate: " << fps << std::endl;
		double duration_sec = num_frames / fps;
		std::cout << "Duration: " << duration_sec << " seconds" << std::endl;

		double format = cap.get(cv::CAP_PROP_FORMAT);
		std::cout << "Format: " << format << std::endl;

		if (frame_start < 0 || frame_start >= num_frames)
			return;
		if (frame_end < 0)
			frame_end = int32_t(num_frames) - 1;
		if (frame_end >= int32_t(num_frames))
			return;
		if (frame_start > frame_end)
			return;

		if (frame_start > 0)
			cap.set(cv::CAP_PROP_POS_FRAMES, frame_start - 1);			// VideoCapture.read() reads the next frame

		if (game_rect.width <= 0 || game_rect.height <= 0)
		{
			game_rect.x = 0;
			game_rect.y = 0;
			game_rect.width = (int)width;
			game_rect.height = (int)height;
		}

		if (game_rect.x < 0 || game_rect.y < 0 || game_rect.x + game_rect.width >(int)width || game_rect.y + game_rect.height >(int)height)
		{
			std::cout << "Error: game image area outside video frame" << std::endl;
			return;
		}
		std::cout << "Game area: (" << game_rect.x << ", " << game_rect.y << ") + (" << game_rect.width << ", " << game_rect.height << ")" << std::endl;

		constexpr double item_box_duration = 3;		// 3 seconds for an item box
		const int32_t item_box_duration_nframe = int32_t(item_box_duration * fps + 0.5);
		std::map<std::string, int32_t> last_item_detected_frame;
		int32_t last_tower_detected_frame = -1;
		std::set<int32_t> scheduled_image_dump_frames;
		for (int32_t frame_number = frame_start; frame_number <= frame_end; frame_number++)
		{
			bool should_dump_image = false;
			DWORD tbegin = ::timeGetTime();
			cv::Mat frame;
			if (!cap.read(frame))
				break;

			int cur_frame = int(cap.get(cv::CAP_PROP_POS_FRAMES));
			char buf[30];
			{
				double sec_lf;
				int frame_in_sec = int(std::modf(cur_frame / fps, &sec_lf) * fps + 0.5);
				int sec = int(sec_lf);
				sprintf_s(buf, "[%d] %02d:%02d:%02d.%02d", cur_frame, sec / 3600, sec % 3600 / 60, sec % 60 , frame_in_sec);
			}

			DWORD tend = ::timeGetTime();
			if (cur_frame % 30 == 0)
				std::cout << buf << " (" << tend - tbegin << " ms)    \r";

			std::string item = item_detector.GetItem(frame(game_rect));
			if (!item.empty())
			{
				auto itor = last_item_detected_frame.find(item);
				if (itor == last_item_detected_frame.end() || itor->second <= cur_frame - item_box_duration_nframe)
				{
					if (itor == last_item_detected_frame.end())
						itor = last_item_detected_frame.emplace(item, cur_frame).first;
					else
						itor->second = cur_frame;
					std::cout << buf << " " << item << std::endl;
					if (ofs.is_open())
						ofs << buf << " " << item << std::endl;
					if (item == "Spirit Orb")
						scheduled_image_dump_frames.insert(cur_frame + int32_t(20 * fps + 0.5));
					else
						should_dump_image = true;
				}
			}

			if (tower_detector.IsActivatingTower(frame(game_rect)))
			{
				if (last_tower_detected_frame < 0 || last_tower_detected_frame <= cur_frame - item_box_duration_nframe)
				{
					last_tower_detected_frame = cur_frame;
					std::cout << buf << " Sheikah Tower activated." << std::endl;
					if (ofs.is_open())
						ofs << buf << " Sheikah Tower activated." << std::endl;
					scheduled_image_dump_frames.insert(cur_frame + int32_t(40 * fps + 0.5));
				}
			}

			if (auto itor = scheduled_image_dump_frames.find(cur_frame) != scheduled_image_dump_frames.end())
			{
				should_dump_image = true;
				scheduled_image_dump_frames.erase(itor);
			}
			if (should_dump_image)
			{
				cv::imwrite(std::to_string(cur_frame) + ".jpg", frame(game_rect));
			}
		}
	}
	else
	{
		std::cout << "Cannot open video file " << video_file << std::endl;
	}
}

bool str_to_int(const std::string& in_str, int& out_int)
{
	std::size_t pos;
	out_int = std::stoi(in_str, &pos);
	return pos == in_str.size();
}

int main(int argc, char* argv[])
{
	// just in case of non-ansi text in console
	::SetConsoleOutputCP(CP_UTF8);

	// for shorter thread time slices
	::timeBeginPeriod(1);

	// disable sleep mode
	::SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);

	bool video_mode = false;
	int frame_start = -1, frame_end = -1;
	std::string video_file_name;
	int bbox_left = 0, bbox_top = 0, bbox_right = -1, bbox_bottom = -1;
	std::string output_file_name;

	if (video_mode)
	{
		std::cout << "Running in video file mode" << std::endl;

		HANDLE hConsole = ::GetStdHandle(STD_OUTPUT_HANDLE);

		AnalyseVideo(video_file_name, cv::Rect(bbox_left, bbox_top, bbox_right - bbox_left + 1, bbox_bottom - bbox_top + 1), frame_start, frame_end, output_file_name);
	}

	return 0;
}
