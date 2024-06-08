#include "location_detector.h"
#include "detector.h"

// Pre-process the location names to make matching easier
[[nodiscard]]
static std::string PreprocessLocationName(const std::string& loc_in)
{
	std::string ret = loc_in;
	// spaces and single quotes are ignored since sometimes they are not correctly recognized
	ret.erase(std::remove_if(ret.begin(), ret.end(), [](auto& c) -> bool { return c == ' ' || c == '\''; }), ret.end());

	// The Hylia Serif font (unofficial name) used by BotW has similar glyphs for upper/lower-case letters, causing the recognition to mix them sometimes.
	// Forcing to uppercase to get rid of this confusion.
	std::for_each(ret.begin(), ret.end(), [](auto& c) { c = std::toupper(c); });

	return ret;
}

LocationDetector::LocationDetector(tesseract::TessBaseAPI& api)
	: _tess_api(api)
{
}

bool LocationDetector::Init(const char* lang)
{
	if (!InitLocationList(lang))
		return false;

	return true;
}

bool LocationDetector::InitLocationList(const char* lang)
{
	std::string shrine_list_file(std::string(lang) + "_locations.txt");
	std::ifstream ifs(shrine_list_file);
	if (!ifs.is_open())
	{
		std::cout << "Cannot open file " << shrine_list_file << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(ifs, line))
		_locations.emplace_back(line, PreprocessLocationName(line));

	return true;
}

std::string LocationDetector::FindBestLocationMatch(const std::string& loc_in)
{
	std::string loc_in_preprocessed = PreprocessLocationName(loc_in);
	uint32_t max_allowed_edits = uint32_t(loc_in_preprocessed.size() / 5);			// allow maximum 1/5 recognition error
	uint32_t candidate_num_edits = max_allowed_edits + 1;
	std::string candidate;
	for (const Location& loc : _locations)
	{
		if (uint32_t(abs(int32_t(loc.preprocessed_name.size()) - int32_t(loc_in_preprocessed.size()))) > max_allowed_edits)
			continue;

		uint32_t num_edits = util::GetStringEditDistance(loc.preprocessed_name, loc_in_preprocessed, max_allowed_edits + 1);
		// prefer shorter names if the editing distance is the same.
		// This is because Tesseract might incorrectly recognize extra random characters inside the location bbox but after the names.
		if (num_edits < candidate_num_edits || (num_edits == candidate_num_edits && candidate.size() > loc.name.size()))
		{
			candidate_num_edits = num_edits;
			candidate = loc.name;
		}
	}
	return candidate;
}

std::string LocationDetector::GetLocation(const cv::Mat& game_img)
{
	// This is the bounding box of the longest location text in the lower left corner of the game screen.
	constexpr double bbox_x0 = 0.038461538461;
	constexpr double bbox_x1 = 0.502652519893;
	constexpr double bbox_y0 = 0.838443396226;
	constexpr double bbox_y1 = 0.926886792452;

	// get the bounding box in rows / cols
	uint32_t bbox_col0 = uint32_t(bbox_x0 * double(game_img.cols) + 0.5);
	uint32_t bbox_col1 = uint32_t(bbox_x1 * double(game_img.cols) + 0.5);
	uint32_t bbox_row0 = uint32_t(bbox_y0 * double(game_img.rows) + 0.5);
	uint32_t bbox_row1 = uint32_t(bbox_y1 * double(game_img.rows) + 0.5);

	// Peek the left-most quarter of the location frame, the shorted location name is "Docks", which is about this wide
	cv::Mat img = game_img(cv::Rect(bbox_col0, bbox_row0, (bbox_col1 - bbox_col0) / 4, bbox_row1 - bbox_row0));
	static const std::vector<Detector::GreyScaleTestCriteria> crit = {
		{.brightness_range_lower = 241, .brightness_range_upper = 255, .pixel_ratio_lower = 0.15, .pixel_ratio_upper = 0.3}
	};
	if (!Detector::GreyscaleTest(img, crit))
		return "";

	double scale_factor = std::max(game_img.cols / 480.0, 1.0);	// according to experiments, it's still possible to recognize the location with high accuracy when the width of the game screen is 480.
	std::string ret = Detector::OCR(game_img(cv::Rect(bbox_col0, bbox_row0, bbox_col1 - bbox_col0, bbox_row1 - bbox_row0)), scale_factor, 180, 255, _tess_api, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'- ");

	// some post-process
	{
		std::string boxes = std::unique_ptr<char[]>(_tess_api.GetBoxText(0)).get();
		std::istringstream boxesstream(boxes);
		std::string letter;
		int letter_x0, letter_y0, letter_x1, letter_y1;
		if (!(boxesstream >> letter >> letter_x0 >> letter_y0 >> letter_x1 >> letter_y1))
			return "";
		if (letter_x0 > int(bbox_row1 - bbox_row0) / 2)		// text not starting from the left side of the location frame, one possibility is that dialog text is recognized (right side of the location bounding-box overlaps with the dialog box)
			return "";
		if (letter_x1 - letter_x0 > int(bbox_row1 - bbox_row0))	// text bounding box is weird-shaped
			return "";
	}

	return FindBestLocationMatch(ret);
}
