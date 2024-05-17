#include "common.h"


class LocationDetector
{
private:
	struct Location
	{
		std::string name;
		std::string preprocessed_name;
	};

private:
	tesseract::TessBaseAPI _tess_api;
	std::vector<Location> _locations;

private:
	bool InitLocationList(const char* lang);

	// Lookup the location list and find the best match for the detected location string
	std::string FindBestLocationMatch(const std::string& loc_in);

public:
	LocationDetector() = default;
	~LocationDetector();
	bool Init(const char* lang);

	// returns empty string if nothing is detected
	std::string GetLocation(const cv::Mat& game_img);
};