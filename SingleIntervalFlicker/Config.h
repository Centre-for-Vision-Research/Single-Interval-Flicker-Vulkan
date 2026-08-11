#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// holds the 4 permuations per image
struct ImagePaths {
    int positionX;
    int positionY;
    std::string name;
    std::string codec; // e.g. "fraunhofer_b", "dsc" -- determines which subfolder under condImageDirectory holds L_dec/R_dec
    fs::path L_orig; // <name>_L_orig.<ext>
    fs::path L_dec; // <name>_L_dec.<ext>
    fs::path R_orig; // <name>_R_orig.<ext>
    fs::path R_dec; // <name>_R_dec.<ext>
    int viewingMode; //0 = stereo   1 = left only   2 = right only
    int flickerIndex = 0; // this tracks whether the first or the second image will be flickered. Populated from the 'answer' column in trials.csv.
};

struct Config {
    fs::path origImageDirectory; // reference images
    fs::path condImageDirectory; // flicker condition images (base dir; codec subfolder resolved per-trial)
    std::vector<ImagePaths> trials;
    fs::path outputDirectory = "C://flickerTestOutput"; // where the results csv is output
    int intervalMode = 1; // 0 = two interval; 1 = single interval (two images, side by side)
    int displayMode = 1; // 0 = SDR only ; 1 = HDR preferred

    // defaults
    double flickerRate = 10.0;  // hz
    double waitTime = 2.0; // time between images
    double imageTime = 2.0; // time images are shown
    int targetFPS = 30;

    // load and parse the json config (settings only -- participant, block, and
    // session info are configured elsewhere)
    bool load(const std::string& configPath);
    // load and parse trials.csv (codec,image_name,answer,x,y,stereo mode) into `trials`
    bool loadTrials(const std::string& trialsPath);

private:
    // searches the provided imageDirectory for a file named "<name><suffix>.*" (any extension).
    //returns the matched path (or empty if not found)
    fs::path findImage(const std::string& name, const std::string& suffix, const fs::path imageDirectory) const;
};