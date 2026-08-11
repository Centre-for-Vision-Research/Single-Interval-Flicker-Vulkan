//#include "framework.h"
#include "resource.h"

#include <windows.h>
#include <commctrl.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#include "ParticipantDialog.h"

/*
    This is for auto generating the combo-box participant drop down in the Dialog box.

*/

// participant information


static std::vector<Participant> g_participants;
static ExperimentInfo g_experimentInfo;


// trim whitespace from text helper

static std::string Trim(const std::string& str)
{
    size_t start = 0;
    size_t end = str.length();

    while (start < end &&
        std::isspace(static_cast<unsigned char>(str[start])))
    {
        start++;
    }

    while (end > start &&
        std::isspace(static_cast<unsigned char>(str[end - 1])))
    {
        end--;
    }

    return str.substr(start, end - start);
}


// load participants.csv

static bool LoadParticipantsCSV(const std::string& filename)
{
    g_participants.clear();

    std::ifstream file(filename);

    if (!file.is_open())
    {
        MessageBoxA(
            nullptr,
            ("Could not open participants file:\n\n" + filename).c_str(),
            "Error",
            MB_OK | MB_ICONERROR
        );

        return false;
    }

    std::string line;
    bool firstLine = true;

    // read every line
    while (std::getline(file, line))
    {
        line = Trim(line);

        // skip the header row (subject_id,age,gender,group)
        if (firstLine)
        {
            firstLine = false;
            continue;
        }

        // ignore blank lines
        if (line.empty())
            continue;

        // ignore comment lines
        if (line[0] == '#')
            continue;

        std::stringstream ss(line);

        std::string id;
        std::string ageString;
        std::string genderString;
        std::string groupString;

        if (!std::getline(ss, id, ','))
            continue;

        if (!std::getline(ss, ageString, ','))
            continue;

        if (!std::getline(ss, genderString, ','))
            continue;

        if (!std::getline(ss, groupString, ','))
            continue;

        id = Trim(id);
        ageString = Trim(ageString);
        genderString = Trim(genderString);
        groupString = Trim(groupString);

        if (id.empty())
            continue;

        Participant participant;

        try
        {
            participant.id = id;
            participant.age = std::stoi(ageString);
            participant.gender = genderString.empty()
                ? 'U'
                : genderString[0];
            participant.group = std::stoi(groupString);
        }
        catch (const std::exception&)
        {
            // malformed row (ie nonnumeric age/group) skip it 
            continue;
        }

        g_participants.push_back(participant);
    }

    return !g_participants.empty();
}


// dialog box procedure

static INT_PTR CALLBACK ParticipantDialogProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        HWND hCombo = GetDlgItem(
            hDlg,
            IDC_PARTICIPANT_COMBO
        );

        // populate participant combobox dropdown
        for (const Participant& participant : g_participants)
        {
            SendMessageA(
                hCombo,
                CB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(
                    participant.id.c_str()
                    )
            );
        }
        // first participant selected by default
        if (!g_participants.empty())
        {
            SendMessage(
                hCombo,
                CB_SETCURSEL,
                0,
                0
            );
        }
        // put focus on participant dropdown
        SetFocus(hCombo);

        return FALSE;
    }


    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            // get the selected participant

            HWND hCombo = GetDlgItem(
                hDlg,
                IDC_PARTICIPANT_COMBO
            );

            int selectedIndex =
                static_cast<int>(
                    SendMessage(
                        hCombo,
                        CB_GETCURSEL,
                        0,
                        0
                    )
                    );

            if (selectedIndex == CB_ERR)
            {
                MessageBox(
                    hDlg,
                    L"Please select a participant.",
                    L"Missing Information",
                    MB_OK | MB_ICONWARNING
                );

                return TRUE;
            }


            //get session #
            HWND hSession = GetDlgItem(
                hDlg,
                IDC_SESSION_EDIT
            );

            wchar_t sessionBuffer[32];

            GetWindowTextW(
                hSession,
                sessionBuffer,
                32
            );

            std::wstring sessionString(sessionBuffer);

            if (sessionString.empty())
            {
                MessageBox(
                    hDlg,
                    L"Please enter a session number.",
                    L"Missing Information",
                    MB_OK | MB_ICONWARNING
                );

                SetFocus(hSession);
                return TRUE;
            }

            int session = _wtoi(sessionBuffer);

            if (session < 0)
            {
                MessageBox(
                    hDlg,
                    L"Session number must be 0 or greater.",
                    L"Invalid Session",
                    MB_OK | MB_ICONWARNING
                );

                SetFocus(hSession);
                return TRUE;
            }


            // get block #

            HWND hBlock = GetDlgItem(
                hDlg,
                IDC_BLOCK_EDIT
            );

            wchar_t blockBuffer[32];

            GetWindowTextW(
                hBlock,
                blockBuffer,
                32
            );

            std::wstring blockString(blockBuffer);

            if (blockString.empty())
            {
                MessageBox(
                    hDlg,
                    L"Please enter a block number.",
                    L"Missing Information",
                    MB_OK | MB_ICONWARNING
                );

                SetFocus(hBlock);
                return TRUE;
            }

            int block = _wtoi(blockBuffer);

            if (block < 0)
            {
                MessageBox(
                    hDlg,
                    L"Block number must be 0 or greater.",
                    L"Invalid Block",
                    MB_OK | MB_ICONWARNING
                );

                SetFocus(hBlock);
                return TRUE;
            }


            // save participant info 

            const Participant& participant = g_participants[selectedIndex];

            g_experimentInfo.participantId = participant.id;
            g_experimentInfo.session = session;
            g_experimentInfo.block = block;

            g_experimentInfo.age = participant.age;
            g_experimentInfo.gender = participant.gender;
            g_experimentInfo.group = participant.group;


            // everything valid

            EndDialog(
                hDlg,
                IDOK
            );

            return TRUE;
        }


        case IDCANCEL:

            EndDialog(
                hDlg,
                IDCANCEL
            );

            return TRUE;
        }

        break;
    }
    }

    return FALSE;
}

// show participant/session dialog


bool ShowParticipantDialog(
    HINSTANCE hInstance,
    ExperimentInfo& result)
{
    // load participant information first
    if (!LoadParticipantsCSV("participants.csv"))
    {
        return false;
    }

    INT_PTR dialogResult = DialogBox(
        hInstance,
        MAKEINTRESOURCE(IDD_PARTICIPANT_DIALOG),
        nullptr,
        ParticipantDialogProc
    );

    if (dialogResult == IDOK)
    {
        result = g_experimentInfo;
        return true;
    }

    return false;
}