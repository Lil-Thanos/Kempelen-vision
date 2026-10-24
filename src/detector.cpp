
#include "detector.h"

static const char* SYMBOLS = "PNBRQKpnbrqk";   // chess.yaml ref entrainement model
static const int   NC = 12;

bool PieceDetector::load(const std::string& onnxPath)
{
    try {
        net_ = cv::dnn::readNetFromONNX(onnxPath);
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        return !net_.empty();
    }
    catch (const cv::Exception& e) {
        std::cerr << "Chargement ONNX impossible : " << e.what() << '\n';
        return false;
    }
}

std::vector<Detection> PieceDetector::detect(const cv::Mat& img, float confThr, float nmsThr)
{
    cv::Mat blob = cv::dnn::blobFromImage(img, 1.0 / 255.0, { 640, 640 },
        cv::Scalar(), /*swapRB*/ true, /*crop*/ false);
    net_.setInput(blob);
    cv::Mat out = net_.forward();              

    const int n = out.size[2];
    const float* d = out.ptr<float>();         
    const float sx = img.cols / 640.f, sy = img.rows / 640.f;

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> classes;

    for (int i = 0; i < n; ++i) {
        int best = 0; float bs = 0.f;
        for (int c = 0; c < NC; ++c) {
            float s = d[(4 + c) * n + i];
            if (s > bs) { bs = s; best = c; }
        }
        if (bs < confThr) continue;

        float cx = d[0 * n + i], cy = d[1 * n + i];
        float w = d[2 * n + i], h = d[3 * n + i];
        boxes.emplace_back(int((cx - w / 2) * sx), int((cy - h / 2) * sy),
            int(w * sx), int(h * sy));
        scores.push_back(bs);
        classes.push_back(best);
    }

    std::vector<int> keep;
    cv::dnn::NMSBoxes(boxes, scores, confThr, nmsThr, keep);  

    std::vector<Detection> res;
    for (int k : keep) res.push_back({ classes[k], scores[k], boxes[k] });
    return res;
}

void PieceDetector::toGrid(const std::vector<Detection>& dets, const cv::Size& sz, char grid[8][8])
{
    float bestConf[8][8] = {};
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) grid[r][c] = '.';

    for (const auto& d : dets) {
        float cx = d.box.x + d.box.width / 2.f;
        float cy = d.box.y + d.box.height / 2.f;
        int col = int(cx / (sz.width / 8.0));
        int row = int(cy / (sz.height / 8.0));
        if (col < 0 || col > 7 || row < 0 || row > 7) continue;
        if (d.conf > bestConf[row][col]) {          // une seule pièce par case, la plus sûre
            bestConf[row][col] = d.conf;
            grid[row][col] = SYMBOLS[d.cls];
        }
    }
}

bool PieceDetector::whiteAtBottom(const char grid[8][8])
{
    double w = 0, b = 0; int nw = 0, nb = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            char p = grid[r][c];
            if (p == '.') continue;
            if (isupper((unsigned char)p)) { w += r; ++nw; }
            else { b += r; ++nb; }
        }
    if (!nw || !nb) return true;
    return (w / nw) > (b / nb);       // les blancs sont plus bas dans l'image // à revoir 
}