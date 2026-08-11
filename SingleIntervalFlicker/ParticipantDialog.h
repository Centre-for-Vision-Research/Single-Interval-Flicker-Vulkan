#pragma once
#pragma once
#include <Windows.h>
#include <string>

struct ExperimentInfo
{
    std::string participantId;
    int session;
    int block;

    int age;
    char gender;
    int group;
};


struct Participant
{
    std::string id;
    int age;
    char gender;
    int group;
};

bool ShowParticipantDialog(
    HINSTANCE hInstance,
    ExperimentInfo& result
);