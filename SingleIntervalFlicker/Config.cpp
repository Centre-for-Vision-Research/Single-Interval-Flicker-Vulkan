#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <Windows.h>
#include "Utils.h"

using json = nlohmann::json;

namespace {

    // Trims whitespace and stray \r (in case a CSV was saved with Windows
    // line endings and read on a system that doesn't collapse them).
    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // Minimal comma splitter. Sufficient for these CSVs since none of the
    // fields contain embedded commas or quotes.
    std::vector<std::string> splitCSVLine(const std::string& line) {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string field;
        while (std::getline(ss, field, ',')) {
            fields.push_back(trim(field));
        }
        return fields;
    }

} // namespace

bool Config::load(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "[Config] Could not open config file: " << configPath << "\n";
        return false;
    }

    json j;
    try {
        file >> j;
    }
    catch (const json::parse_error& e) {
        std::cerr << "[Config] JSON parse error: " << e.what() << "\n";
        return false;
    }

    origImageDirectory = j.at("Reference Image Directory").get<std::string>();
    condImageDirectory = j.at("Condition Image Directory").get<std::string>();

    // these are technically 'optional' as they have hardcoded defaults in the config struct
    if (j.contains("Output Directory")) {
        outputDirectory = j["Output Directory"].get<std::string>();
    }
    if (j.contains("Flicker Rate (Hz)")) {
        flickerRate = j["Flicker Rate (Hz)"].get<double>();
    }
    if (j.contains("Wait Time (s)")) {
        waitTime = j["Wait Time (s)"].get<double>();
    }
    if (j.contains("Image Time (s)")) {
        imageTime = j["Image Time (s)"].get<double>();
    }
    if (j.contains("Target FPS")) {
        targetFPS = j["Target FPS"].get<int>();
    }
    if (j.contains("Display Mode")) {
        displayMode = j["Display Mode"].get<int>();
    }
    if (j.contains("Interval Mode")) {
        intervalMode = j["Interval Mode"].get<int>();
    }

    if (!fs::exists(origImageDirectory) || !fs::is_directory(origImageDirectory)) {
        std::string msg = "[Config] Image directory not found: " + origImageDirectory.string();
        Utils::FatalError(msg);
        return false;
    }

    if (!fs::exists(condImageDirectory) || !fs::is_directory(condImageDirectory)) {
        std::string msg = "[Config] Image directory not found: " + condImageDirectory.string();
        Utils::FatalError(msg);
        return false;
    }

    return true;
}

bool Config::loadTrials(const std::string& trialsPath) {
    std::ifstream file(trialsPath);
    if (!file.is_open()) {
        std::cerr << "[Config] Could not open trials file: " << trialsPath << "\n";
        return false;
    }

    trials.clear();

    std::string line;
    if (!std::getline(file, line)) {
        std::cerr << "[Config] Trials file is empty: " << trialsPath << "\n";
        return false;
    }
    // expected header: codec,image_name,answer,x,y,stereo mode

    int lineNumber = 1;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (trim(line).empty()) continue;

        std::vector<std::string> fields = splitCSVLine(line);
        if (fields.size() < 6) {
            std::string msg = "[Config] Malformed trials.csv line " + std::to_string(lineNumber) + ": " + line;
            Utils::FatalError(msg);
            continue;
        }

        std::string codec;
        std::string imageName;
        int flickerIndex = 0, positionX = 0, positionY = 0, stereoMode = 0;

        try {
            codec = fields[0];
            imageName = fields[1];
            flickerIndex = std::stoi(fields[2]); // 'answer' column
            positionX = std::stoi(fields[3]);
            positionY = std::stoi(fields[4]);
            stereoMode = std::stoi(fields[5]);
        }
        catch (const std::exception& e) {
            std::string msg = "[Config] Could not parse trials.csv line " + std::to_string(lineNumber) + ": " + e.what();
            Utils::FatalError(msg);
            continue;
        }

        // codec determines which subfolder holds the condition images
        // e.g. condImageDirectory / "fraunhofer_b" / image0_L.*
        fs::path codecDirectory = condImageDirectory / codec;
        if (!fs::exists(codecDirectory) || !fs::is_directory(codecDirectory)) {
            std::string msg = "[Config] Codec directory not found: " + codecDirectory.string();
            Utils::FatalError(msg);
            continue;
        }

        ImagePaths img;
        img.name = imageName;
        img.codec = codec;
        img.flickerIndex = flickerIndex;
        img.positionX = positionX;
        img.positionY = positionY;
        img.viewingMode = stereoMode;

        img.L_orig = findImage(imageName, "_L", origImageDirectory);
        img.R_orig = findImage(imageName, "_R", origImageDirectory);
        img.L_dec = findImage(imageName, "_L", codecDirectory);
        img.R_dec = findImage(imageName, "_R", codecDirectory);

        // warn about any missing permutations
        auto warn = [&](const fs::path& p, const std::string& suffix, const fs::path& dir) {
            if (p.empty()) {
                std::string msg = "[Config] Warning: no file found for \"" + imageName + suffix + ".*\" in " + dir.string();
                Utils::FatalError(msg);
            }
            };

        warn(img.L_orig, "_L", origImageDirectory);
        warn(img.R_orig, "_R", origImageDirectory);
        warn(img.L_dec, "_L", codecDirectory);
        warn(img.R_dec, "_R", codecDirectory);

        trials.push_back(img);
    }

    return true;
}

fs::path Config::findImage(const std::string& name, const std::string& suffix, const fs::path imageDirectory) const {
    std::string stem = name + suffix; // e.g. "image0_L"

    for (const auto& entry : fs::directory_iterator(imageDirectory)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (filename.rfind(stem, 0) == 0)  // starts with stem
            return entry.path();
    }

    return {};
}