#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

// voir entrainement du modele 

struct Detection {
    int cls;          
    float conf;
    cv::Rect box;     
};

class PieceDetector {

public:
    bool        load(const std::string& onnxPath);

    std::vector<Detection>      detect(const cv::Mat& boardBgr, float confThr = 0.4f, float nmsThr = 0.5f);

    static void          toGrid(const std::vector<Detection>& dets, const cv::Size& cropSize, char grid[8][8]);
    static bool          whiteAtBottom(const char grid[8][8]);

private:
    cv::dnn::Net net_;
};