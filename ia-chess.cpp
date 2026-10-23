#include "ia-chess.h"

#include <opencv2/core/utils/logger.hpp>


using namespace std;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

	FrameCapture frameCapture;

	cout << "Welcome to Kempelen" << endl;

	frameCapture.captureFrame();


	return 0;
}
