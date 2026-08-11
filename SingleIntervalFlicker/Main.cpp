#include "App.h"
#include "ParticipantDialog.h"

#include <Windows.h>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: App.exe <config.json> <participants.csv> <trials.csv>\n";
        return -1;
    }


    std::string configPath = (argc > 1) ? argv[1] : "./config.json";

    std::string participantsPath = (argc > 2) ? argv[2] : "./participants.csv";

    std::string trialsPath = (argc > 3) ? argv[3] : "./trials.csv";

    App app;

   

    // ask for participat/session/block information

    ExperimentInfo experimentInfo;

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    if (!ShowParticipantDialog(hInstance, experimentInfo)) // cancelled
    {
        std::cout << "Experiment cancelled.\n";
        return 0;
    }

    // start experiment


    if (!app.init(configPath, trialsPath, experimentInfo))
    {
        std::cerr
            << "Failed to initialize app with config: "
            << configPath
            << "\n";

        return -1;
    }

    app.run();

    return 0;
}