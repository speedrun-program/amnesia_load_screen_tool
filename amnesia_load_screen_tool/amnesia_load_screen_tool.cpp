
// To compile this, remember to add winhttp.lib to additional dependencies in project properties.
// (Configuration Properties -> Linker -> Input -> Additional Dependencies)

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winhttp.h>
#include <winternl.h>

#include "file_helper.h"            // FileHelper class
#include "process_helper.h"         // ProcessHelper class
#include "update_check.h"           // isMostRecentVersion function
#include "injected_instructions.h"  // mainMenuDelayInstructions, mapDelayInstructions, flashbackSkipInstructions, and flashbackWaitInstructions


typedef LONG(__stdcall* NTFUNCTION)(HANDLE);


const wchar_t steamName[] = L"Amnesia.exe";
const wchar_t noSteamName[] = L"Amnesia_NoSteam.exe";


struct InjectionInfo {

    // locations in Amnesia's memory
    uint32_t gpBaseLocation = 0;
    uint32_t cSoundEntryStopLocation = 0;
    uint32_t beforeFadeOutAllLocation = 0; // jump location
    uint32_t engineRunLocation = 0;        // jump location
    uint32_t getStepSizeLocation = 0;
    uint32_t altf4QuitLocation = 0;        // jump location
    uint32_t noSaveQuitLocation = 0;       // jump location
    uint32_t saveQuitLocation = 0;         // jump location
    uint32_t quickloadingLocation = 0;     // jump location if quickloading, then jumps over the loadFromMenuLocation2 jump
    uint32_t loadingFromMenuLocation = 0;  // jump location if not quickloading
    uint32_t DestroyMapLocation = 0;
    uint32_t injectedInstructionsLocation = 0;
    uint32_t injectedDataLocation = 0;
    uint32_t slipperyPhysicsLocation = 0;

    // info found by reading flashback_names.txt, maps_and_delays.txt, and amnesia_settings.txt
    uint32_t howManyFlashbackNames = 0;
    uint32_t lengthOfLongestFlashbackName = 0;
    uint32_t lengthOfCommonPrefix = 0;
    uint32_t howManyMapNames = 0;
    uint32_t lengthOfLongestMapName = 0;
    uint32_t mainMenuDelay = 0;
    uint32_t spaceForCommonPrefix = 0;
    uint32_t spacePerFlashbackName = 0;
    uint32_t sizeOfFlashbackNameArea = 0;
    uint32_t spacePerMapName = 0;
    uint32_t sizeOfMapsAndDelaysArea = 0;
    uint32_t spaceForInstructions = 0;
    bool skippingFlashBacks = false;
    unsigned char secondsRemainingBeforeUnwait[sizeof(double)] = { 0 };

    // copied instructions from Amnesia's memory
    unsigned char sleepCallBytes[6] = { 0 };
    unsigned char strncmpCallBytes[6] = { 0 };
    unsigned char beforeFadeOutAllBytes[6] = { 0 };
    unsigned char altf4QuitBytes[6] = { 0 };
    unsigned char noSaveQuitBytes[6] = { 0 };
    unsigned char saveQuitBytes[6] = { 0 };
    unsigned char loadingFromMenuBytes[7] = { 0 };

    // offsets of data and virtual functions
    unsigned char gpBaseMpSoundOffset = 0;
    unsigned char mpSoundHandlerOffset = 0;
    unsigned char m_lstSoundEntriesOffset = 0;
    unsigned char nodeCSoundEntryOffset = 0;
    unsigned char soundChannelOffset = 0;
    unsigned char isPlayingOffset = 0;
    unsigned char getPausedOffset = 0;
    unsigned char getLoopingOffset = 0;
    unsigned char getTotalTimeOffset = 0;
    unsigned char getElapsedTimeOffset = 0;

    bool delayingMainMenu = false;
};


template <const size_t circularBufferSize>
class CircularBuffer {

public:

    static_assert(
        circularBufferSize && ((circularBufferSize & (circularBufferSize - 1)) == 0),
        "circularBufferSize needs to be a power of two and greater than zero.\n"
        );

    unsigned char buffer[circularBufferSize] = { 0 };
    size_t start = 0;

    unsigned char operator[](const size_t idx) const {

        return buffer[(idx + start) & (sizeof(buffer) - 1)];
    }

    void addToEnd(const unsigned char newEndValue) {

        buffer[start] = newEndValue;
        start = (start + 1) & (sizeof(buffer) - 1);
    }

    void copyBytes(unsigned char* destination, const size_t startIdx, const size_t howManyBytes) const {

        for (size_t i = 0; i < howManyBytes; i++) {
            destination[i] = buffer[(start + i + startIdx) & (sizeof(buffer) - 1)];
        }
    }
};


static void getExitInput(const bool succeeded) {

    int ch = 0;
    printf("%sPress Enter to close this window.\n", succeeded ? "Amnesia successfully injected.\n" : "Couldn't inject Amnesia.\n");
    ch = getchar();
}


static DWORD searchUsingSnapshotHandle(PROCESSENTRY32* processEntry, const HANDLE snapshot, bool* isSteamVersion) {

    if (!Process32First(snapshot, processEntry)) {
        printf("Error when using Process32First (error code %u).\n", GetLastError());

        return (DWORD)-1;
    }

    do {
        if ((*isSteamVersion = (wcscmp(processEntry->szExeFile, steamName) == 0)) || wcscmp(processEntry->szExeFile, noSteamName) == 0) {
            return processEntry->th32ProcessID;
        }
    } while (Process32Next(snapshot, processEntry));

    return (DWORD)-1;
}


static DWORD findAmnesiaPid(bool* isSteamVersion) {

    DWORD amnesiaPid = (DWORD)-1;

    PROCESSENTRY32 processEntry = { 0 };
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);                // resource acquired

    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Error when using CreateToolhelp32Snapshot (error code %u).\n", GetLastError());
        return amnesiaPid;
    }

    amnesiaPid = searchUsingSnapshotHandle(&processEntry, snapshot, isSteamVersion);
    CloseHandle(snapshot);                                                            // resource released

    if (amnesiaPid == (DWORD)-1) {
        printf("Couldn't find amnesia process PID.\n");
    }

    return amnesiaPid;
}


static bool findNtFunctions(NTFUNCTION* NtSuspendProcess, NTFUNCTION* NtResumeProcess) {

    HMODULE ntdllHandle = GetModuleHandle(L"ntdll.dll");

    if (!ntdllHandle) {
        printf("Error using GetModuleHandle to find ntdll.dll (error code %u).\n", GetLastError());

        return false;
    }

    *NtSuspendProcess = (NTFUNCTION)GetProcAddress(ntdllHandle, "NtSuspendProcess");

    if (!*NtSuspendProcess) {
        printf("Error using GetProcAddress to find NtSuspendProcess (error code %u)\n.", GetLastError());

        return false;
    }

    *NtResumeProcess = (NTFUNCTION)GetProcAddress(ntdllHandle, "NtResumeProcess");

    if (!*NtResumeProcess) {
        printf("Error using GetProcAddress to find NtResumeProcess (error code %u).\n", GetLastError());

        return false;
    }

    return true;
}


static char* checkIfYesOrNo(const char* s, bool* setting, bool* settingReadSuccessfully) {

    *settingReadSuccessfully = true;
    
    s += strspn(s, " \f\t\v\r");

    if (*s == 'y' || *s == 'Y' || *s == 't' || *s == 'T' || *s == '1') {
        *setting = true;
    } else if (*s == 'n' || *s == 'N' || *s == 'f' || *s == 'F' || *s == '0') {
        *setting = false;
    } else {
        *settingReadSuccessfully = false;
    }

    return (char*)s;
}


static bool readSettingsFile(
        bool* skippingFlashbacks,
        bool* delayMaps,
        bool* enableSlipperyPhysics,
        bool* allowUnexpectedGameVersions,
        bool* checkForToolUpdates,
        bool* allowNotFullyUpdatedTool,
        double* secondsRemainingBeforeUnwait,
        DWORD* millisecondsUpdateCheckTimeout) {

    char buffer[512] = { 0 };

    const char defaultText[] = "skip flashbacks: n\r\n\
delay maps: y\r\n\
enable consistent slippery physics: y\r\n\
allow unexpected game versions: n\r\n\
check for tool updates: y\r\n\
allow not fully updated tool: n\r\n\
milliseconds remaining before unwait: 435\r\n\
milliseconds update check timeout: 5000\r\n";

    static_assert(sizeof(defaultText) < sizeof(buffer), "amnesia_settings.txt defaultText is too big for buffer.\n");

    const char settingsFileName[] = "amnesia_settings.txt";

    const char nameOfSkipFlashbacksSetting[] = "skip flashbacks:";
    const char nameOfDelayMapsSetting[] = "delay maps:";
    const char nameOfEnableSlipperyPhysics[] = "enable consistent slippery physics:";
    const char nameOfAllowUnexpectedGameVersions[] = "allow unexpected game versions:";
    const char nameOfCheckForToolUpdates[] = "check for tool updates:";
    const char nameOfAllowNotFullyUpdatedTool[] = "allow not fully updated tool:";
    const char nameOfMillisecondsRemainingBeforeUnwait[] = "milliseconds remaining before unwait:";
    const char nameOfMillisecondsUpdateCheckTimeout[] = "milliseconds update check timeout:";

    bool skipFlashbacksSettingFound = false;
    bool delayMapsSettingFound = false;
    bool enableSlipperyPhysicsSettingFound = false;
    bool allowUnexpectedGameVersionsSettingFound = false;
    bool checkForToolUpdatesSettingFound = false;
    bool allowNotFullyUpdatedToolSettingFound = false;

    FILE* f = nullptr;

    if (fopen_s(&f, settingsFileName, "rb") != 0 || !f) {    // resource acquired (1)
        printf("Couldn't open %s.\n", settingsFileName);

        return false;
    }

    size_t bytesRead = fread(buffer, 1, sizeof(buffer), f);

    fclose(f);                                               // resource released (1)

    f = nullptr;

    // make sure bytesRead is less than sizeof(buffer) so the buffer text is null-terminated
    if (bytesRead == sizeof(buffer)) {
        printf("%s should be smaller than %zu bytes.\nResetting %s.\n", settingsFileName, sizeof(buffer), settingsFileName);

        if (fopen_s(&f, settingsFileName, "wb") != 0 || !f) { // resource acquired (2)
            printf("Couldn't open %s.\n", settingsFileName);

            return false;
        }

        fwrite(defaultText, 1, sizeof(defaultText) - 1, f);  // - 1 because the null character isn't needed

        fclose(f);                                           // resource released (2)

        return false;
    }

    char* strtolWaitStartPtr = nullptr;
    char* strtolWaitEndPtr = nullptr;
    char* strtolTimeoutStartPtr = nullptr;
    char* strtolTimeoutEndPtr = nullptr;
    long millisecondsRemainingBeforeUnwait = 0;
    long millisecondsUpdateCheckTimeoutLong = 0;

    for (char* bufferPtr = buffer; bufferPtr != nullptr && *bufferPtr != '\0'; bufferPtr = strchr(bufferPtr, '\n')) {
        
        bufferPtr += strspn(bufferPtr, " \f\t\v\r\n"); // going to the start of the next line in the file

        if (strncmp(bufferPtr, nameOfSkipFlashbacksSetting, sizeof(nameOfSkipFlashbacksSetting) - 1) == 0) {

            bufferPtr = checkIfYesOrNo(
                bufferPtr + sizeof(nameOfSkipFlashbacksSetting) - 1,
                skippingFlashbacks,
                &skipFlashbacksSettingFound
            );

        } else if (strncmp(bufferPtr, nameOfDelayMapsSetting, sizeof(nameOfDelayMapsSetting) - 1) == 0) {

            bufferPtr = checkIfYesOrNo(
                bufferPtr + sizeof(nameOfDelayMapsSetting) - 1,
                delayMaps,
                &delayMapsSettingFound
            );

        } else if (strncmp(bufferPtr, nameOfEnableSlipperyPhysics, sizeof(nameOfEnableSlipperyPhysics) - 1) == 0) {

            bufferPtr = checkIfYesOrNo(
                bufferPtr + sizeof(nameOfEnableSlipperyPhysics) - 1,
                enableSlipperyPhysics,
                &enableSlipperyPhysicsSettingFound
            );

        } else if (strncmp(bufferPtr, nameOfAllowUnexpectedGameVersions, sizeof(nameOfAllowUnexpectedGameVersions) - 1) == 0) {

            bufferPtr = checkIfYesOrNo(
                bufferPtr + sizeof(nameOfAllowUnexpectedGameVersions) - 1,
                allowUnexpectedGameVersions,
                &allowUnexpectedGameVersionsSettingFound
            );

        } else if (strncmp(bufferPtr, nameOfCheckForToolUpdates, sizeof(nameOfCheckForToolUpdates) - 1) == 0) {

            bufferPtr = checkIfYesOrNo(
                bufferPtr + sizeof(nameOfCheckForToolUpdates) - 1,
                checkForToolUpdates,
                &checkForToolUpdatesSettingFound
            );

        } else if (strncmp(bufferPtr, nameOfAllowNotFullyUpdatedTool, sizeof(nameOfAllowNotFullyUpdatedTool) - 1) == 0) {

            bufferPtr = checkIfYesOrNo(
                bufferPtr + sizeof(nameOfAllowNotFullyUpdatedTool) - 1,
                allowNotFullyUpdatedTool,
                &allowNotFullyUpdatedToolSettingFound
            );

        } else if (strncmp(bufferPtr, nameOfMillisecondsRemainingBeforeUnwait, sizeof(nameOfMillisecondsRemainingBeforeUnwait) - 1) == 0) {

            strtolWaitStartPtr = bufferPtr + sizeof(nameOfMillisecondsRemainingBeforeUnwait) - 1;
            millisecondsRemainingBeforeUnwait = strtol(strtolWaitStartPtr, &strtolWaitEndPtr, 10);
            bufferPtr = strtolWaitEndPtr;

        } else if (strncmp(bufferPtr, nameOfMillisecondsUpdateCheckTimeout, sizeof(nameOfMillisecondsUpdateCheckTimeout) - 1) == 0) {

            strtolTimeoutStartPtr = bufferPtr + sizeof(nameOfMillisecondsUpdateCheckTimeout) - 1;
            millisecondsUpdateCheckTimeoutLong = strtol(strtolTimeoutStartPtr, &strtolTimeoutEndPtr, 10);
            bufferPtr = strtolTimeoutEndPtr;

        }
    }

    if (strtolWaitStartPtr == strtolWaitEndPtr) {
        printf("Couldn't read \"milliseconds remaining before unwait\" setting.\n");
        millisecondsRemainingBeforeUnwait = 0;
    } else if (millisecondsRemainingBeforeUnwait <= 0) {
        printf("Milliseconds remaining before unwait must be greater than zero.\n");
    }

    if (millisecondsUpdateCheckTimeoutLong >= 4294967295) {
        printf("Milliseconds update check timeout must be less than than 4294967295.\n");
        millisecondsUpdateCheckTimeoutLong = 0;
    } else if (strtolTimeoutStartPtr == strtolTimeoutEndPtr) {
        printf("Couldn't read \"milliseconds update check timeout\" setting.\n");
        millisecondsUpdateCheckTimeoutLong = 0;
    } else if (millisecondsUpdateCheckTimeoutLong <= 0) {
        printf("Milliseconds update check timeout must be greater than zero.\n");
    }

    *secondsRemainingBeforeUnwait = millisecondsRemainingBeforeUnwait / 1000.0;

    *millisecondsUpdateCheckTimeout = (DWORD)millisecondsUpdateCheckTimeoutLong;

    if (!(
            skipFlashbacksSettingFound
            && delayMapsSettingFound
            && enableSlipperyPhysicsSettingFound
            && allowUnexpectedGameVersionsSettingFound
            && checkForToolUpdatesSettingFound
            && allowNotFullyUpdatedToolSettingFound
            && (millisecondsRemainingBeforeUnwait > 0)
            && (millisecondsUpdateCheckTimeoutLong > 0)
        )) {

        printf("Couldn't read all settings in %s.\nResetting %s.\n", settingsFileName, settingsFileName);

        if (fopen_s(&f, settingsFileName, "wb") != 0 || !f) { // resource acquired (3)
            printf("Couldn't open %s.\n", settingsFileName);

            return false;
        }

        fwrite(defaultText, 1, sizeof(defaultText) - 1, f);  // - 1 because the null character isn't needed

        fclose(f);                                           // resource released (3)

        return false;
    }
    
    return true;
}


static bool preprocessFlashbackNamesFile(
        uint32_t* howManyFlashbackNames,
        uint32_t* lengthOfLongestFlashbackName,
        uint32_t* lengthOfCommonPrefix,
        const uint32_t maxcommonPrefixSize, // this should be one less than the commonPrefix buffer size
        char* commonPrefix) {

    FileHelper fh("flashback_names.txt");
    
    if (!fh.f) {
        return false;
    }

    char ch = '\0';

    // the first name needs to be copied to commonPrefix
    for (bool keepReading = true; *howManyFlashbackNames == 0 && keepReading;) {
        while ((keepReading = fh.getCharacter(&ch))) {
            ch += (32 * (ch >= 'A' && ch <= 'Z')); // changing ch to lowercase because amnesia stores flashback names in lowercase

            if (ch == '\r') {
                continue;
            } else if (ch == '\n') {
                *howManyFlashbackNames += (*lengthOfLongestFlashbackName != 0);
                break;
            } else {
                if (*lengthOfLongestFlashbackName < maxcommonPrefixSize) {
                    commonPrefix[*lengthOfLongestFlashbackName] = ch;
                    *lengthOfCommonPrefix += 1;
                }

                *lengthOfLongestFlashbackName += 1;
            }
        }
    }

    commonPrefix[(*lengthOfLongestFlashbackName <= maxcommonPrefixSize) ? *lengthOfLongestFlashbackName : maxcommonPrefixSize] = '\0';

    uint32_t currentFlashbackNameLength = 0;

    while (fh.getCharacter(&ch)) {
        if (ch == '\r') {
            continue;
        } else if (ch == '\n') {
            if (currentFlashbackNameLength == 0) { // it was an empty line
                continue;
            }
            if (*lengthOfLongestFlashbackName < currentFlashbackNameLength) {
                *lengthOfLongestFlashbackName = currentFlashbackNameLength;
            }
            if (currentFlashbackNameLength < *lengthOfCommonPrefix) {
                commonPrefix[currentFlashbackNameLength] = '\0';
                *lengthOfCommonPrefix = currentFlashbackNameLength;
            }
            *howManyFlashbackNames += 1;
            currentFlashbackNameLength = 0;
        } else {
            if (currentFlashbackNameLength < *lengthOfCommonPrefix && commonPrefix[currentFlashbackNameLength] != ch) {
                commonPrefix[currentFlashbackNameLength] = '\0';
                *lengthOfCommonPrefix = currentFlashbackNameLength;
            }
            currentFlashbackNameLength += 1;
        }
    }

    // last line
    if (*lengthOfLongestFlashbackName < currentFlashbackNameLength) {
        *lengthOfLongestFlashbackName = currentFlashbackNameLength;
    }

    *howManyFlashbackNames += (currentFlashbackNameLength != 0);

    return true;
}


static bool preprocessMapDelaysFile(uint32_t* howManyMapNames, uint32_t* lengthOfLongestMapName, bool* delayingMainMenu) {

    FileHelper fh("maps_and_delays.txt");

    if (!fh.f) {
        return false;
    }

    char ch = '\0';
    uint32_t currentMapNameLength = 0;

    while (fh.getCharacter(&ch)) {
        if (ch == '\r') {
            continue;
        } else if (ch == '/' || ch == '\n') {
            if (*lengthOfLongestMapName < currentMapNameLength) {
                *lengthOfLongestMapName = currentMapNameLength;
            }

            if (ch == '/' || currentMapNameLength == 0) {
                *delayingMainMenu = true;
            }

            *howManyMapNames += (currentMapNameLength != 0);
            currentMapNameLength = 0;

            while (ch != '\n' && fh.getCharacter(&ch)); // finishing reading the line
        } else {
            currentMapNameLength += 1;
        }
    }

    // last line
    if (*lengthOfLongestMapName < currentMapNameLength) {
        *lengthOfLongestMapName = currentMapNameLength;
    }

    *howManyMapNames += (currentMapNameLength != 0);

    return true;
}


static bool findInstructions(InjectionInfo* ii, ProcessHelper* ph) {

    unsigned char b = 0;
    bool alreadyInjected = false;
    size_t instructionPatternsFound = 0;
    unsigned char locationCopyBytes[sizeof(uint32_t)] = { 0 };
    CircularBuffer<128> memorySlice;

    for (size_t i = sizeof(memorySlice.buffer) - 1; i != 0; i--) {
        ph->getByte(&b);
        memorySlice.addToEnd(b);
    }

    // finding where to write to and copy from in amnesia's memory based on instruction byte patterns
    for (size_t currentMemoryAddress = ph->textSegmentLocation; ph->getByte(&b); currentMemoryAddress++) {
        memorySlice.addToEnd(b);

        if (memorySlice[0] == 0x8b && memorySlice[1] == 0x88 && memorySlice[6] == 0xe8 && memorySlice[11] == 0x8b && memorySlice[14] == 0x51) {

            if (memorySlice[26] == 0xe8 || memorySlice[26] == 0xe9) { // call or jmp
                alreadyInjected = true;
                break;
            }

            uint32_t beforeFadeOutAllLocation = currentMemoryAddress + 26;
            memcpy(&ii->beforeFadeOutAllLocation, &beforeFadeOutAllLocation, sizeof(ii->beforeFadeOutAllLocation));
            memorySlice.copyBytes(ii->beforeFadeOutAllBytes, 26, sizeof(ii->beforeFadeOutAllBytes));
            memorySlice.copyBytes(locationCopyBytes, 34, sizeof(locationCopyBytes));
            memcpy(&ii->gpBaseLocation, locationCopyBytes, sizeof(ii->gpBaseLocation));
            ii->gpBaseMpSoundOffset = memorySlice[42];
            ii->mpSoundHandlerOffset = memorySlice[45];
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0xd9 && memorySlice[4] == 0x8b && memorySlice[10] == 0x6a && memorySlice[11] == 0x05) {

            ii->engineRunLocation = currentMemoryAddress + 20;
            memorySlice.copyBytes(locationCopyBytes, 21, sizeof(locationCopyBytes));
            memcpy(&ii->getStepSizeLocation, locationCopyBytes, sizeof(ii->getStepSizeLocation));
            ii->getStepSizeLocation += ii->engineRunLocation + 5;

        } else if (memorySlice[0] == 0x8b && memorySlice[1] == 0x46 && memorySlice[3] == 0x8b && memorySlice[4] == 0x10 && memorySlice[5] == 0x3b && memorySlice[6] == 0xd0) {

            ii->m_lstSoundEntriesOffset = memorySlice[2];
            ii->nodeCSoundEntryOffset = memorySlice[19];
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x56 && memorySlice[1] == 0x8b && memorySlice[7] == 0x75 && memorySlice[9] == 0x80) {

            uint32_t cSoundEntryStopLocation = currentMemoryAddress;
            memcpy(&ii->cSoundEntryStopLocation, &cSoundEntryStopLocation, sizeof(ii->cSoundEntryStopLocation));
            ii->soundChannelOffset = memorySlice[17];
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0xd8 && memorySlice[1] == 0x80 && memorySlice[10] == 0x80) {
            
            ii->isPlayingOffset = memorySlice[37];
            ii->getPausedOffset = memorySlice[49];
            ii->getLoopingOffset = memorySlice[68];
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x05 && memorySlice[5] == 0x8b && memorySlice[12] == 0x17) {

            ii->getTotalTimeOffset = memorySlice[28];
            ii->getElapsedTimeOffset = memorySlice[55];
            instructionPatternsFound += 1;
            
        } else if (memorySlice[0] == 0x6a && memorySlice[1] == 0x0a && memorySlice[2] == 0xff && memorySlice[8] == 0x8b) {
            
            memorySlice.copyBytes(ii->sleepCallBytes, 2, sizeof(ii->sleepCallBytes));
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x6a && memorySlice[1] == 0x05 && memorySlice[7] == 0x53) {
            
            memorySlice.copyBytes(ii->strncmpCallBytes, 8, sizeof(ii->strncmpCallBytes));
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x46 && memorySlice[2] == 0x53 && memorySlice[3] == 0x50 && memorySlice[4] == 0x8b && memorySlice[6] == 0xe8) {
            
            if (memorySlice[73] == 0xe8) { // call
                alreadyInjected = true;
                break;
            }

            uint32_t altf4QuitLocation = currentMemoryAddress + 73;
            memcpy(&ii->altf4QuitLocation, &altf4QuitLocation, sizeof(ii->altf4QuitLocation));
            memorySlice.copyBytes(ii->altf4QuitBytes, 73, sizeof(ii->altf4QuitBytes));
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x75 && memorySlice[2] == 0x8b && memorySlice[3] == 0xcf && memorySlice[9] == 0x68) {
            
            if (memorySlice[30] == 0xe8) { // call
                alreadyInjected = true;
                break;
            }

            uint32_t noSaveQuitLocation = currentMemoryAddress + 30;
            memcpy(&ii->noSaveQuitLocation, &noSaveQuitLocation, sizeof(ii->noSaveQuitLocation));
            memorySlice.copyBytes(ii->noSaveQuitBytes, 30, sizeof(ii->noSaveQuitBytes));
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x8b && memorySlice[1] == 0x8a && memorySlice[11] == 0x68) {
            
            if (memorySlice[32] == 0xe8) { // call
                alreadyInjected = true;
                break;
            }

            uint32_t saveQuitLocation = currentMemoryAddress + 32;
            memcpy(&ii->saveQuitLocation, &saveQuitLocation, sizeof(ii->saveQuitLocation));
            memorySlice.copyBytes(ii->saveQuitBytes, 32, sizeof(ii->saveQuitBytes));
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x53 && memorySlice[1] == 0x68 && memorySlice[6] == 0xe8 && memorySlice[14] == 0x85) {
            
            if (memorySlice[33] == 0xe9) { // jmp
                alreadyInjected = true;
                break;
            }

            uint32_t quickloadingLocation = currentMemoryAddress + 33;
            uint32_t loadingFromMenuLocation = currentMemoryAddress + 40;
            memcpy(&ii->quickloadingLocation, &quickloadingLocation, sizeof(ii->quickloadingLocation));
            memcpy(&ii->loadingFromMenuLocation, &loadingFromMenuLocation, sizeof(ii->loadingFromMenuLocation));
            memorySlice.copyBytes(ii->loadingFromMenuBytes, 38, sizeof(ii->loadingFromMenuBytes));
            memorySlice.copyBytes(locationCopyBytes, 34, sizeof(locationCopyBytes));
            memcpy(&ii->DestroyMapLocation, locationCopyBytes, sizeof(ii->DestroyMapLocation));
            ii->DestroyMapLocation += currentMemoryAddress + 38;
            instructionPatternsFound += 1;

        } else if (memorySlice[0] == 0x9a && memorySlice[5] == 0x8b && memorySlice[6] == 0x07) {

            uint32_t slipperyPhysicsLocation = currentMemoryAddress + 7;
            memcpy(&ii->slipperyPhysicsLocation, &slipperyPhysicsLocation, sizeof(ii->slipperyPhysicsLocation));

        }
    }

    if (alreadyInjected) {
        printf("Amnesia is already injected.\n");

        return false;
    }
    if (instructionPatternsFound > 12) {
        printf("Duplicate instruction patterns were found.\n");

        return false;
    }
    if (!(
        ii->gpBaseLocation != 0
        && ii->engineRunLocation != 0
        && ii->m_lstSoundEntriesOffset != 0
        && ii->cSoundEntryStopLocation != 0
        && ii->isPlayingOffset != 0
        && ii->getTotalTimeOffset != 0
        && ii->sleepCallBytes[0] == 0xff && ii->sleepCallBytes[1] == 0x15
        && ii->strncmpCallBytes[0] == 0xff && ii->strncmpCallBytes[1] == 0x15
        && ii->altf4QuitLocation != 0
        && ii->noSaveQuitLocation != 0
        && ii->saveQuitLocation != 0
        && ii->quickloadingLocation != 0
        && ii->loadingFromMenuLocation != 0
        && ii->slipperyPhysicsLocation != 0
        )) {
        printf("Couldn't find all instruction patterns.\n");

        return false;
    }
    
    return true;
}


static bool injectFlashbackNames(ProcessHelper* ph, const InjectionInfo* ii) {

    FileHelper fh("flashback_names.txt");

    if (!fh.f) {
        return false;
    }

    char ch = '\0';
    
    // base loop terminations on how many bytes have been written in case flashback_names.txt size was somehow changed
    bool keepReading = true;

    for (uint32_t namesWritten = 0; keepReading && namesWritten < ii->howManyFlashbackNames;) {
        uint32_t sectionPosition = 0;

        // going past the common prefix part
        if (ii->lengthOfCommonPrefix != 0) {
            for (uint32_t i = ii->lengthOfCommonPrefix; i != 0 && (keepReading = fh.getCharacter(&ch)) && ch != '\n'; i--);

            // line ended, probably because this is an empty line
            if (ch == '\n') {
                continue;
            }
        }

        // writing the flashback name past the common prefix
        while ((keepReading = fh.getCharacter(&ch)) && ch != '\n' && sectionPosition < ii->lengthOfLongestFlashbackName) {
            if (ch == '\r') {
                continue;
            }

            if (!ph->writeByte((unsigned char)ch)) {
                return false;
            }

            sectionPosition += 1;
        }

        // finishing reading the line
        while (ch != '\n' && (keepReading = fh.getCharacter(&ch)));

        // filling in the remaining space with 0x00 bytes
        // there should always be at least one 0x00 byte
        if (sectionPosition != 0 || ii->lengthOfCommonPrefix != 0) {
            for (; sectionPosition < ii->spacePerFlashbackName; sectionPosition++) {
                if (!ph->writeByte(0x00)) {
                    return false;
                }
            }

            namesWritten += 1;
        }
    }

    return true;
}


static bool injectMapNamesAndDelays(ProcessHelper* ph, InjectionInfo* ii) {

    FileHelper fh("maps_and_delays.txt");

    if (!fh.f) {
        return false;
    }

    bool menuDelayWritten = false;
    char ch = '\0';
    
    // base loop terminates based on how many bytes have been written in case maps_and_delays.txt size was somehow changed
    bool keepReading = true;

    for (uint32_t namesWritten = 0; keepReading && namesWritten < ii->howManyMapNames;) {
        uint32_t sectionPosition = 0;
        uint32_t delay = 0;

        // writing the map name
        while (sectionPosition < ii->lengthOfLongestMapName && (keepReading = fh.getCharacter(&ch)) && ch != '/' && ch != '\n') {
            if (ch == '\r') {
                continue;
            }

            if (!ph->writeByte((unsigned char)ch)) {
                return false;
            }

            sectionPosition += 1;
        }

        if (ch == '\n' && sectionPosition == 0) { // this was an empty line
            continue;
        }

        // going to the digits
        while (ch != '\n' && !(ch >= '0' && ch <= '9') && (keepReading = fh.getCharacter(&ch)));

        // determining the delay time
        if (ch >= '0' && ch <= '9') {
            do {
                delay *= 10;
                delay += ch - '0';

                if (delay > 0xffffff) { // ensure at least one byte stays 0x00 in case strncmp somehow reads into this value
                    printf("Map delays can't be more than 0xffffff.\n");

                    return false;
                }
            } while ((keepReading = fh.getCharacter(&ch)) && (ch >= '0' && ch <= '9'));
        }

        // finishing reading the line
        while (ch != '\n' && (keepReading = fh.getCharacter(&ch)));

        if (sectionPosition == 0) { // this line had the main menu delay, so set ii->mainMenuDelay to delay
            ii->mainMenuDelay = delay;
            menuDelayWritten = true;
        } else {
            // filling in the remaining space with 0x00 bytes
            // there should always be at least one 0x00 byte
            for (; sectionPosition < ii->spacePerMapName - sizeof(uint32_t); sectionPosition++) {
                if (!ph->writeByte(0x00)) {
                    return false;
                }
            }

            // writing the delay
            for (size_t i = 0; i < sizeof(uint32_t); i++) {
                if (!ph->writeByte(((unsigned char*)(&delay))[i])) {
                    return false;
                }
            }

            namesWritten += 1;
        }
    }

    // the main menu delay wasn't found, probably because it's at the end
    while (ii->delayingMainMenu && !menuDelayWritten && keepReading) {
        fh.getCharacter(&ch); // getting the line's first character

        if (ch != '/') {
            while (ch != '\n' && (keepReading = fh.getCharacter(&ch))); // going to the next line
        } else {
            menuDelayWritten = true;

            // going to the digits
            while (ch != '\n' && !(ch >= '0' && ch <= '9') && (keepReading = fh.getCharacter(&ch)));

            // determining the delay time
            if (ch >= '0' && ch <= '9') {
                uint32_t delay = 0;

                do {
                    delay *= 10;
                    delay += ch - '0';

                    if (delay > 0xffffff) { // ensure at least one byte stays 0x00 in case strncmp somehow reads into this value
                        printf("Map delays can't be more than 0xffffff.\n");

                        return false;
                    }
                } while ((keepReading = fh.getCharacter(&ch)) && (ch >= '0' && ch <= '9'));

                ii->mainMenuDelay = delay;
            }
        }
    }

    return true;
}


static bool injectData(ProcessHelper* ph, InjectionInfo* ii, const char* commonPrefix) {

    // writing the common prefix
    for (size_t i = ii->lengthOfCommonPrefix; i != 0; i--) {
        if (!ph->writeByte(*commonPrefix)) {
            return false;
        }
        commonPrefix += 1;
    }

    if (ii->skippingFlashBacks) {
        // moving to the flashback names bytes
        for (size_t i = ii->spaceForCommonPrefix - ii->lengthOfCommonPrefix; i != 0; i--) {
            if (!ph->writeByte(0x00)) {
                return false;
            }
        }
    } else {
        // moving to the ii->secondsRemainingBeforeUnwait bytes
        for (size_t i = ii->spaceForCommonPrefix - ii->lengthOfCommonPrefix - sizeof(double); i != 0; i--) {
            if (!ph->writeByte(0x00)) {
                return false;
            }
        }

        // writing ii->secondsRemainingBeforeUnwait
        for (size_t i = 0; i < sizeof(ii->secondsRemainingBeforeUnwait); i++) {
            if (!ph->writeByte(ii->secondsRemainingBeforeUnwait[i])) {
                return false;
            }
        }
    }

    if (!injectFlashbackNames(ph, ii)) {
        return false;
    }

    if (ii->howManyMapNames != 0) {
        if (!injectMapNamesAndDelays(ph, ii)) {
            return false;
        }
    }

    // writing anything still in the buffer
    return ph->flushBuffer();
}


static void prepareMainMenuDelayInstructions(const InjectionInfo* ii, unsigned char* instructionBufferPtr) {

    memcpy(instructionBufferPtr, mainMenuDelayInstructions, sizeof(mainMenuDelayInstructions));

    memcpy(&instructionBufferPtr[1], &ii->mainMenuDelay, sizeof(ii->mainMenuDelay));
    memcpy(&instructionBufferPtr[20], &ii->mainMenuDelay, sizeof(ii->mainMenuDelay));
    memcpy(&instructionBufferPtr[40], &ii->mainMenuDelay, sizeof(ii->mainMenuDelay));

    memcpy(&instructionBufferPtr[5], ii->sleepCallBytes, sizeof(ii->sleepCallBytes));
    memcpy(&instructionBufferPtr[24], ii->sleepCallBytes, sizeof(ii->sleepCallBytes));
    memcpy(&instructionBufferPtr[44], ii->sleepCallBytes, sizeof(ii->sleepCallBytes));

    memcpy(&instructionBufferPtr[11], &ii->altf4QuitBytes, sizeof(ii->altf4QuitBytes));
    memcpy(&instructionBufferPtr[31], &ii->noSaveQuitBytes, sizeof(ii->noSaveQuitBytes));
    memcpy(&instructionBufferPtr[51], &ii->saveQuitBytes, sizeof(ii->saveQuitBytes));
}


static void prepareMapDelayInstructions(const InjectionInfo* ii, unsigned char* instructionBufferPtr) {

    instructionBufferPtr += sizeof(mainMenuDelayInstructions);

    memcpy(instructionBufferPtr, mapDelayInstructions, sizeof(mapDelayInstructions));

    uint32_t mapDelayInstructionsStart = ii->injectedInstructionsLocation + sizeof(mainMenuDelayInstructions);
    uint32_t firstMapNameAddress = ii->injectedDataLocation + ii->spaceForCommonPrefix + (ii->spacePerFlashbackName * ii->howManyFlashbackNames);
    uint32_t noMoreMapNamesAddress = firstMapNameAddress + (ii->spacePerMapName * ii->howManyMapNames);
    uint32_t delayOffset = ii->spacePerMapName - sizeof(uint32_t);
    uint32_t DestroyMapCallOffset = ii->DestroyMapLocation - (mapDelayInstructionsStart + 5);
    uint32_t backToAmnesiaOffset1 = (ii->loadingFromMenuLocation + sizeof(ii->loadingFromMenuBytes) - 2) - (mapDelayInstructionsStart + 17);
    uint32_t backToAmnesiaOffset2 = (ii->loadingFromMenuLocation + sizeof(ii->loadingFromMenuBytes) - 2) - (mapDelayInstructionsStart + 123);

    memcpy(&instructionBufferPtr[1], &DestroyMapCallOffset, sizeof(DestroyMapCallOffset));
    memcpy(&instructionBufferPtr[5], ii->loadingFromMenuBytes, sizeof(ii->loadingFromMenuBytes));
    memcpy(&instructionBufferPtr[13], &backToAmnesiaOffset1, sizeof(backToAmnesiaOffset1));
    memcpy(&instructionBufferPtr[22], &ii->spacePerMapName, sizeof(ii->spacePerMapName));
    memcpy(&instructionBufferPtr[28], &ii->strncmpCallBytes[2], sizeof(uint32_t));
    memcpy(&instructionBufferPtr[43], &noMoreMapNamesAddress, sizeof(noMoreMapNamesAddress));
    memcpy(&instructionBufferPtr[49], &firstMapNameAddress, sizeof(firstMapNameAddress));
    memcpy(&instructionBufferPtr[97], &delayOffset, sizeof(delayOffset));
    memcpy(&instructionBufferPtr[101], ii->sleepCallBytes, sizeof(ii->sleepCallBytes));
    memcpy(&instructionBufferPtr[113], &ii->loadingFromMenuBytes[2], sizeof(ii->loadingFromMenuBytes) - 2);
    memcpy(&instructionBufferPtr[119], &backToAmnesiaOffset2, sizeof(backToAmnesiaOffset2));
}


static void prepareFlashbackSkipInstructions(const InjectionInfo* ii, unsigned char* instructionBufferPtr) {

    instructionBufferPtr += sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions);

    memcpy(instructionBufferPtr, flashbackSkipInstructions, sizeof(flashbackSkipInstructions));

    uint32_t flashbackSkipInstructionsStart = ii->injectedInstructionsLocation + sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions);
    uint32_t commonPrefixAddress = ii->injectedDataLocation;
    uint32_t firstFlashbackNameAddress = commonPrefixAddress + ii->spaceForCommonPrefix;
    uint32_t noMoreFlashbackNamesAddress = firstFlashbackNameAddress + (ii->spacePerFlashbackName * ii->howManyFlashbackNames);
    uint32_t cSoundEntryStopOffset = ii->cSoundEntryStopLocation - (flashbackSkipInstructionsStart + 159);
    uint32_t backToAmnesiaOffset = (ii->beforeFadeOutAllLocation + sizeof(ii->beforeFadeOutAllBytes) + 14) - (flashbackSkipInstructionsStart + 201);

    memcpy(&instructionBufferPtr[33], &ii->gpBaseLocation, sizeof(ii->gpBaseLocation));
    instructionBufferPtr[41] = ii->gpBaseMpSoundOffset;
    instructionBufferPtr[44] = ii->mpSoundHandlerOffset;
    instructionBufferPtr[51] = ii->m_lstSoundEntriesOffset;
    memcpy(&instructionBufferPtr[54], &ii->strncmpCallBytes[2], sizeof(uint32_t));
    instructionBufferPtr[66] = ii->nodeCSoundEntryOffset;
    memcpy(&instructionBufferPtr[83], &ii->spacePerFlashbackName, sizeof(ii->spacePerFlashbackName));
    memcpy(&instructionBufferPtr[89], &ii->lengthOfCommonPrefix, sizeof(ii->lengthOfCommonPrefix));
    memcpy(&instructionBufferPtr[95], &commonPrefixAddress, sizeof(commonPrefixAddress));
    memcpy(&instructionBufferPtr[107], &ii->lengthOfCommonPrefix, sizeof(ii->lengthOfCommonPrefix));
    memcpy(&instructionBufferPtr[112], &firstFlashbackNameAddress, sizeof(firstFlashbackNameAddress));
    memcpy(&instructionBufferPtr[123], &noMoreFlashbackNamesAddress, sizeof(noMoreFlashbackNamesAddress));
    memcpy(&instructionBufferPtr[155], &cSoundEntryStopOffset, sizeof(cSoundEntryStopOffset));
    memcpy(&instructionBufferPtr[164], &ii->spacePerFlashbackName, sizeof(ii->spacePerFlashbackName));
    memcpy(&instructionBufferPtr[171], &noMoreFlashbackNamesAddress, sizeof(noMoreFlashbackNamesAddress));
    memcpy(&instructionBufferPtr[190], ii->beforeFadeOutAllBytes, sizeof(ii->beforeFadeOutAllBytes));
    memcpy(&instructionBufferPtr[197], &backToAmnesiaOffset, sizeof(backToAmnesiaOffset));
}


static void prepareFlashbackWaitInstructions(const InjectionInfo* ii, unsigned char* instructionBufferPtr) {

    instructionBufferPtr += sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions);

    memcpy(instructionBufferPtr, flashbackWaitInstructions, sizeof(flashbackWaitInstructions));

    uint32_t flashbackWaitInstructionsStart = ii->injectedInstructionsLocation + sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions);
    uint32_t commonPrefixAddress = ii->injectedDataLocation;
    uint32_t firstFlashbackNameAddress = commonPrefixAddress + ii->spaceForCommonPrefix;
    uint32_t secondsRemainingBeforeUnwaitAddress = firstFlashbackNameAddress - sizeof(double);
    uint32_t waitForFlashbackByteLocation = secondsRemainingBeforeUnwaitAddress - 1;
    uint32_t noMoreFlashbackNamesAddress = firstFlashbackNameAddress + (ii->spacePerFlashbackName * ii->howManyFlashbackNames);
    uint32_t getStepSizeOffset = ii->getStepSizeLocation - (flashbackWaitInstructionsStart + 21);

    memcpy(&instructionBufferPtr[4], &waitForFlashbackByteLocation, sizeof(waitForFlashbackByteLocation));
    memcpy(&instructionBufferPtr[8], ii->beforeFadeOutAllBytes, sizeof(ii->beforeFadeOutAllBytes));
    memcpy(&instructionBufferPtr[17], &getStepSizeOffset, sizeof(getStepSizeOffset));
    memcpy(&instructionBufferPtr[22], &waitForFlashbackByteLocation, sizeof(waitForFlashbackByteLocation));
    memcpy(&instructionBufferPtr[41], &waitForFlashbackByteLocation, sizeof(waitForFlashbackByteLocation));
    memcpy(&instructionBufferPtr[46], &ii->gpBaseLocation, sizeof(ii->gpBaseLocation));
    instructionBufferPtr[54] = ii->gpBaseMpSoundOffset;
    instructionBufferPtr[57] = ii->mpSoundHandlerOffset;
    instructionBufferPtr[60] = ii->m_lstSoundEntriesOffset;
    memcpy(&instructionBufferPtr[74], &ii->strncmpCallBytes[2], sizeof(uint32_t));
    instructionBufferPtr[86] = ii->nodeCSoundEntryOffset;
    memcpy(&instructionBufferPtr[107], &ii->spacePerFlashbackName, sizeof(ii->spacePerFlashbackName));
    memcpy(&instructionBufferPtr[113], &ii->lengthOfCommonPrefix, sizeof(ii->lengthOfCommonPrefix));
    memcpy(&instructionBufferPtr[119], &commonPrefixAddress, sizeof(commonPrefixAddress));
    memcpy(&instructionBufferPtr[131], &ii->lengthOfCommonPrefix, sizeof(ii->lengthOfCommonPrefix));
    memcpy(&instructionBufferPtr[136], &firstFlashbackNameAddress, sizeof(firstFlashbackNameAddress));
    memcpy(&instructionBufferPtr[147], &noMoreFlashbackNamesAddress, sizeof(noMoreFlashbackNamesAddress));
    instructionBufferPtr[182] = ii->soundChannelOffset;
    instructionBufferPtr[193] = ii->getPausedOffset;
    instructionBufferPtr[196] = ii->getLoopingOffset;
    instructionBufferPtr[201] = ii->isPlayingOffset;
    instructionBufferPtr[210] = ii->getElapsedTimeOffset;
    instructionBufferPtr[219] = ii->getTotalTimeOffset;
    memcpy(&instructionBufferPtr[247], &ii->spacePerFlashbackName, sizeof(ii->spacePerFlashbackName));
    memcpy(&instructionBufferPtr[254], &noMoreFlashbackNamesAddress, sizeof(noMoreFlashbackNamesAddress));
    memcpy(&instructionBufferPtr[278], &secondsRemainingBeforeUnwaitAddress, sizeof(secondsRemainingBeforeUnwaitAddress));
    memcpy(&instructionBufferPtr[290], &ii->sleepCallBytes, sizeof(ii->sleepCallBytes));
}


static bool writeToProcessWithCacheFlush(
        const ProcessHelper* ph,
        const uint32_t writeLocation,
        const uint32_t howManyBytesToWrite,
        const unsigned char* bytesToWrite,
        bool* terminateAmnesia) {

    uint32_t bytesWritten = ph->writeToProcess(writeLocation, bytesToWrite, howManyBytesToWrite);

    if (bytesWritten != howManyBytesToWrite) {

        if (bytesWritten != 0) {
            printf("WARNING: Instruction at memory address 0x%08x only partially overwritten.\n", writeLocation);

            *terminateAmnesia = true;
        }

        return false;
    }

    if (!FlushInstructionCache(ph->processHandle, (LPCVOID)writeLocation, howManyBytesToWrite)) {
        printf(
            "WARNING: Couldn't flush %u bytes in instruction cache starting at memory address 0x%08x (error code %u).\n",
            howManyBytesToWrite,
            writeLocation,
            GetLastError()
        );

        *terminateAmnesia = true;

        return false;
    }

    return true;
}


static bool injectOriginalMemory(
        const ProcessHelper* ph,
        const InjectionInfo* ii,
        const bool enableSlipperyPhysics,
        bool* terminateAmnesia) {

    unsigned char jmp[15] = { 0xe9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    unsigned char call[15] = { 0xe8, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

    unsigned char slipperyPhysicsBytes[3] = {
        0x31, 0xc9,    // xor ecx, ecx
        0x90           // nop
    };

    /////////////// injecting main menu delay calls ///////////////
    uint32_t altf4QuitInstructionsStart = ii->injectedInstructionsLocation;
    uint32_t noSaveQuitInstructionsStart = altf4QuitInstructionsStart + 18;
    uint32_t saveQuitInstructionsStart = altf4QuitInstructionsStart + 38;

    uint32_t fromAltf4QuitOffset = altf4QuitInstructionsStart - (ii->altf4QuitLocation + 5);
    uint32_t fromNoSaveQuitOffset = noSaveQuitInstructionsStart - (ii->noSaveQuitLocation + 5);
    uint32_t fromSaveQuitOffset = saveQuitInstructionsStart - (ii->saveQuitLocation + 5);

    if (ii->mainMenuDelay != 0) {

        memcpy(&call[1], &fromAltf4QuitOffset, sizeof(fromAltf4QuitOffset));

        if (!writeToProcessWithCacheFlush(ph, ii->altf4QuitLocation, sizeof(ii->altf4QuitBytes), call, terminateAmnesia)) {
            return false;
        }

        memcpy(&call[1], &fromNoSaveQuitOffset, sizeof(fromNoSaveQuitOffset));

        if (!writeToProcessWithCacheFlush(ph, ii->noSaveQuitLocation, sizeof(ii->noSaveQuitBytes), call, terminateAmnesia)) {
            return false;
        }

        memcpy(&call[1], &fromSaveQuitOffset, sizeof(fromSaveQuitOffset));

        if (!writeToProcessWithCacheFlush(ph, ii->saveQuitLocation, sizeof(ii->saveQuitBytes), call, terminateAmnesia)) {
            return false;
        }

    }
    ///////////////////////////////////////////////////////////////

    ////////////////// injecting map delay jmps ///////////////////
    uint32_t quickloadInstructionsStart = ii->injectedInstructionsLocation + sizeof(mainMenuDelayInstructions);
    uint32_t menuLoadInstructionsStart = quickloadInstructionsStart + 18;

    uint32_t fromQuickloadOffset = quickloadInstructionsStart - (ii->quickloadingLocation + 5);
    uint32_t fromMenuLoadOffset = menuLoadInstructionsStart - (ii->loadingFromMenuLocation + 5);

    if (ii->howManyMapNames != 0) {
        
        memcpy(&jmp[1], &fromQuickloadOffset, sizeof(fromQuickloadOffset));

        if (!writeToProcessWithCacheFlush(ph, ii->quickloadingLocation, 5, jmp, terminateAmnesia)) {
            return false;
        }

        memcpy(&jmp[1], &fromMenuLoadOffset, sizeof(fromMenuLoadOffset));

        if (!writeToProcessWithCacheFlush(ph, ii->loadingFromMenuLocation, 5, jmp, terminateAmnesia)) {
            return false;
        }

    }
    ///////////////////////////////////////////////////////////////

    ///// injecting flashback skip or flashback wait call(s) //////
    if (ii->howManyFlashbackNames != 0) {

        if (ii->skippingFlashBacks) {

            uint32_t flashbackSkipInstructionsStart = ii->injectedInstructionsLocation + sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions) + 32;

            uint32_t fromBeforeFadeOutAllOffset = flashbackSkipInstructionsStart - (ii->beforeFadeOutAllLocation + 5);

            memcpy(&jmp[1], &fromBeforeFadeOutAllOffset, sizeof(fromBeforeFadeOutAllOffset));

            if (!writeToProcessWithCacheFlush(ph, ii->beforeFadeOutAllLocation, sizeof(ii->beforeFadeOutAllBytes), jmp, terminateAmnesia)) {
                return false;
            }

        } else {

            uint32_t byteToggleInstructionsStart = ii->injectedInstructionsLocation + sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions);
            uint32_t afterByteToggleInstructionsStart = byteToggleInstructionsStart + 16;

            uint32_t fromBeforeFadeOutAllOffset = byteToggleInstructionsStart - (ii->beforeFadeOutAllLocation + 5);
            uint32_t fromGetStepSizeCallLocationOffset = afterByteToggleInstructionsStart - (ii->engineRunLocation + 5);

            memcpy(&call[1], &fromBeforeFadeOutAllOffset, sizeof(fromBeforeFadeOutAllOffset));

            if (!writeToProcessWithCacheFlush(ph, ii->beforeFadeOutAllLocation, sizeof(ii->beforeFadeOutAllBytes), call, terminateAmnesia)) {
                return false;
            }

            memcpy(&call[1], &fromGetStepSizeCallLocationOffset, sizeof(fromGetStepSizeCallLocationOffset));

            if (!writeToProcessWithCacheFlush(ph, ii->engineRunLocation, 5, call, terminateAmnesia)) {
                return false;
            }

        }

    }
    ///////////////////////////////////////////////////////////////

    ////////////// injecting slippery physics bytes ///////////////
    if (
        enableSlipperyPhysics
        && !writeToProcessWithCacheFlush(ph, ii->slipperyPhysicsLocation, sizeof(slipperyPhysicsBytes), slipperyPhysicsBytes, terminateAmnesia)
        ) {
        return false;
    }
    ///////////////////////////////////////////////////////////////

    return true;
}


static bool injectDataAndInstructions(
    ProcessHelper* ph,
    InjectionInfo* ii,
    const char* commonPrefix,
    const bool enableSlipperyPhysics,
    bool* terminateAmnesia,
    NTFUNCTION NtSuspendProcess,
    NTFUNCTION NtResumeProcess) {
    
    if (!injectData(ph, ii, commonPrefix)) {
        return false;
    }
    
    unsigned char instructionBuffer[1024] = { 0 };

    static_assert(
        sizeof(flashbackWaitInstructions) >= sizeof(flashbackSkipInstructions),
        "flashback wait instructions are smaller than flashback skip instructions, so update the static_assert here"
    );

    static_assert(
        sizeof(instructionBuffer) >= (sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions) + sizeof(flashbackWaitInstructions)),
        "InstructionBuffer isn't big enough.\n"
    );

    if (ii->mainMenuDelay != 0) {
        prepareMainMenuDelayInstructions(ii, instructionBuffer);
    }

    if (ii->howManyMapNames != 0) {
        prepareMapDelayInstructions(ii, instructionBuffer);
    }

    if (ii->howManyFlashbackNames != 0) {
        if (ii->skippingFlashBacks) {
            prepareFlashbackSkipInstructions(ii, instructionBuffer);
        } else {
            prepareFlashbackWaitInstructions(ii, instructionBuffer);
        }
    }

    ph->writeToProcess(ii->injectedInstructionsLocation, instructionBuffer, sizeof(instructionBuffer));

    DWORD mandatoryArgument = 0;

    if (!VirtualProtectEx(
        ph->processHandle,
        (LPVOID)ii->injectedInstructionsLocation,
        ii->spaceForInstructions,
        PAGE_EXECUTE,
        &mandatoryArgument)) {
        printf("Error when giving the injected instructions area PAGE_EXECUTE protection with VirtualProtectEx (error code %u).\n", GetLastError());
        
        return false;
    }

    NTSTATUS ntFunctionStatus = NtSuspendProcess(ph->processHandle);

    if (!NT_SUCCESS(ntFunctionStatus)) {
        printf("NtSuspendProcess couldn't suspend Amnesia (NTSTATUS code 0x%08x).\n", ntFunctionStatus);

        return false;
    }

    // do this last in case anything else fails
    bool originalMemoryInjectedSuccessfully = injectOriginalMemory(ph, ii, enableSlipperyPhysics, terminateAmnesia);

    ntFunctionStatus = NtResumeProcess(ph->processHandle);

    if (!NT_SUCCESS(ntFunctionStatus)) {
        printf("NtResumeProcess couldn't resume Amnesia (NTSTATUS code 0x%08x).\n", ntFunctionStatus);

        return false;
    }
    
    return originalMemoryInjectedSuccessfully;
}


int main() {

    InjectionInfo ii;

    NTFUNCTION NtSuspendProcess = nullptr;
    NTFUNCTION NtResumeProcess = nullptr;

    if (!findNtFunctions(&NtSuspendProcess, &NtResumeProcess)) {
        getExitInput(false);
        return EXIT_FAILURE;
    }

    bool isSteamVersion = false;
    DWORD amnesiaPid = findAmnesiaPid(&isSteamVersion);

    if (amnesiaPid == (DWORD)-1) {
        getExitInput(false);
        return EXIT_FAILURE;
    }

    const wchar_t* amnesiaName = isSteamVersion ? steamName : noSteamName;
    ProcessHelper ph(amnesiaPid, amnesiaName);

    if (ph.textSegmentLocation == 0) {
        getExitInput(false);
        return EXIT_FAILURE;
    }

    bool skippingFlashbacks = false;
    bool delayMaps = false;
    bool enableSlipperyPhysics = false;
    bool allowUnexpectedGameVersions = false;
    bool checkForToolUpdates = false;
    bool allowNotFullyUpdatedTool = false;
    double secondsRemainingBeforeUnwait = 0.0;
    DWORD millisecondsUpdateCheckTimeout = 0;

    if (!readSettingsFile(
        &skippingFlashbacks,
        &delayMaps,
        &enableSlipperyPhysics,
        &allowUnexpectedGameVersions,
        &checkForToolUpdates,
        &allowNotFullyUpdatedTool,
        &secondsRemainingBeforeUnwait,
        &millisecondsUpdateCheckTimeout
        )) {
        getExitInput(false);
        return EXIT_FAILURE;
    }

    ii.skippingFlashBacks = skippingFlashbacks;
    
    // determining if this version of the tool is the most recent version //
    if (checkForToolUpdates) {
        printf(
            "Checking for updates (timeout is %u milliseconds). To skip this, change the \"check for tool updates\" setting to \"n\", \"f\", or \"0\".\n",
            millisecondsUpdateCheckTimeout
        );

        bool isMostRecentVersionResult = false;

        if (!isMostRecentVersion(&isMostRecentVersionResult, millisecondsUpdateCheckTimeout)) {
            printf("Couldn't determine if this version of the tool is the most recent version.\n");
        }

        if (isMostRecentVersionResult) {
            printf("This is the most recent version of this tool.\n");
        } else if (!allowNotFullyUpdatedTool) {
            printf("To use this tool when it isn't, or might not be, the most recent version, change the \"allow not fully updated tool\" setting to \"y\", \"t\", or \"1\".\n");

            getExitInput(false);
            return EXIT_FAILURE;
        }
    }
    ////////////////////////////////////////////////////////////////////////
    
    if ((!isSteamVersion && ph.remainingBytesToRead != 6467584) || (isSteamVersion && ph.remainingBytesToRead != 6479872)) {
        printf(
            "\
WARNING: %ls's .text segment is %u bytes, but this tool was made for versions which are 6467584 bytes and 6479872 bytes.\n\
This tool might not work correctly with other versions of the game.\n%s",
            amnesiaName,
            ph.remainingBytesToRead,
            allowUnexpectedGameVersions ? "" : "To use this tool with other versions of Amnesia, change the \"allow unexpected game versions\" setting to \"y\", \"t\", or \"1\".\n"
        );
        if (!allowUnexpectedGameVersions) {
            getExitInput(false);
            return EXIT_FAILURE;
        }
    }

    memcpy(&ii.secondsRemainingBeforeUnwait, &secondsRemainingBeforeUnwait, sizeof(double));

    uint32_t howManyFlashbackNames = 0;
    uint32_t lengthOfLongestFlashbackName = 0;
    uint32_t lengthOfCommonPrefix = 0;
    char commonPrefix[320] = { 0 };

    if (!preprocessFlashbackNamesFile(
        &howManyFlashbackNames,
        &lengthOfLongestFlashbackName,
        &lengthOfCommonPrefix,
        sizeof(commonPrefix) - 1,
        commonPrefix
        )) {
        getExitInput(false);
        return EXIT_FAILURE;
    }

    uint32_t howManyMapNames = 0;
    uint32_t lengthOfLongestMapName = 0;
    bool delayingMainMenu = false;

    if (delayMaps) {
        if (!preprocessMapDelaysFile(&howManyMapNames, &lengthOfLongestMapName, &delayingMainMenu)) {
            getExitInput(false);
            return EXIT_FAILURE;
        }
    }

    if (!findInstructions(&ii, &ph)) {
        getExitInput(false);
        return EXIT_FAILURE;
    }

    ///// finding how much space to allocate in Amnesia /////
    
    // space for flashback name area
    // ii.secondsRemainingBeforeUnwait is also stored here at the last 8 bytes of the common prefix area
    // a byte used to check if a flashback might be happening is also stored in the common prefix area behind ii.secondsRemainingBeforeUnwait
    uint32_t spaceForCommonPrefix = lengthOfCommonPrefix + 1;

    // if flashbacks aren't being skipped, add space for ii.secondsRemainingBeforeUnwait plus the byte that signals if a loading screen is happening.
    spaceForCommonPrefix += (sizeof(ii.secondsRemainingBeforeUnwait) * (!skippingFlashbacks)) + (!skippingFlashbacks);

    spaceForCommonPrefix = ((spaceForCommonPrefix / 16) + ((spaceForCommonPrefix % 16) != 0)) * 16;

    uint32_t spacePerFlashbackName = lengthOfLongestFlashbackName - lengthOfCommonPrefix;
    spacePerFlashbackName = (((spacePerFlashbackName + 1) / 16) + (((spacePerFlashbackName + 1) % 16) != 0)) * 16;
    uint32_t sizeOfFlashbackNameArea = (spacePerFlashbackName * howManyFlashbackNames) + spaceForCommonPrefix;

    // space for maps and delays area
    uint32_t spacePerMapName = 0;
    uint32_t sizeOfMapsAndDelaysArea = 0;

    if (delayMaps) {
        spacePerMapName = (((lengthOfLongestMapName + 1 + sizeof(uint32_t)) / 16) + (((lengthOfLongestMapName + 1 + sizeof(uint32_t)) % 16) != 0)) * 16;
        sizeOfMapsAndDelaysArea = spacePerMapName * howManyMapNames;
    }

    // getting the page size
    SYSTEM_INFO sysInfo = { 0 };
    GetSystemInfo(&sysInfo);
    DWORD pageSize = sysInfo.dwPageSize;

    // space for instructions
    static_assert(
        sizeof(flashbackWaitInstructions) >= sizeof(flashbackSkipInstructions),
        "flashback wait instructions are smaller than flashback skip instructions, so update the size calculation here"
    );

    uint32_t sizeOfInstructionArea = sizeof(mainMenuDelayInstructions) + sizeof(mapDelayInstructions) + sizeof(flashbackWaitInstructions);
    uint32_t spaceForInstructions = ((sizeOfInstructionArea / pageSize) + ((sizeOfInstructionArea % pageSize) != 0)) * pageSize;

    // total space
    uint32_t totalSpaceNeeded = spaceForInstructions + sizeOfMapsAndDelaysArea + sizeOfFlashbackNameArea;

    /////////////////////////////////////////////////////////

    ii.howManyFlashbackNames = howManyFlashbackNames;
    ii.lengthOfLongestFlashbackName = lengthOfLongestFlashbackName;
    ii.lengthOfCommonPrefix = lengthOfCommonPrefix;
    ii.howManyMapNames = howManyMapNames;
    ii.lengthOfLongestMapName = lengthOfLongestMapName;
    ii.spaceForCommonPrefix = spaceForCommonPrefix;
    ii.spacePerFlashbackName = spacePerFlashbackName;
    ii.sizeOfFlashbackNameArea = sizeOfFlashbackNameArea;
    ii.spacePerMapName = spacePerMapName;
    ii.sizeOfMapsAndDelaysArea = sizeOfMapsAndDelaysArea;
    ii.spaceForInstructions = spaceForInstructions;
    ii.delayingMainMenu = delayingMainMenu;
    
    LPVOID extraMemoryPtr = VirtualAllocEx(        // resource acquired
        ph.processHandle,
        nullptr,
        totalSpaceNeeded,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (extraMemoryPtr == nullptr) {
        printf("Error when using VirtualAllocEx (error code %u).\n", GetLastError());

        getExitInput(false);
        return false;
    }
    ii.injectedInstructionsLocation = (uint32_t)extraMemoryPtr;
    ii.injectedDataLocation = ii.injectedInstructionsLocation + spaceForInstructions;

    // terminate Amnesia if the jmps and calls were written unsafely (partial writes or instruction cache couldn't be flushed).
    bool terminateAmnesia = false;

    // preparing ph for writing
    ph.whereToReadOrWrite = ii.injectedDataLocation;
    ph.bufferPosition = 0;
    
    if (!injectDataAndInstructions(&ph, &ii, commonPrefix, enableSlipperyPhysics, &terminateAmnesia, NtSuspendProcess, NtResumeProcess)) {
        DWORD lastErrorCode = GetLastError(); // saving the error from injectDataAndInstructions for TerminateProcess

        if (!VirtualFreeEx(ph.processHandle, extraMemoryPtr, 0, MEM_RELEASE)) { // resource released
            printf("WARNING: Error when using VirtualFreeEx (error code %u).\nCouldn't release VirtualAllocEx memory.\n", GetLastError());
        }

        if (terminateAmnesia) {
            printf("Terminating Amnesia.\n");
            
            if (!TerminateProcess(ph.processHandle, lastErrorCode)) {
                printf(
                    "WARNING: Error when using TerminateProcess to close %ls (error code %u).\nCouldn't close %ls. This session of %ls may crash.\n",
                    amnesiaName,
                    GetLastError(),
                    amnesiaName,
                    amnesiaName
                );
            } else {
                printf("%ls was closed to prevent it from crashing.\n", amnesiaName);
            }
        }

        getExitInput(false);
        return EXIT_FAILURE;
    }

    getExitInput(true);

    return EXIT_SUCCESS;
}
