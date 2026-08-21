
#pragma once


const char thisVersionDate[] = "2026-08-14";


// variables shared between the main thread and the startup update check thread
static DWORD GLOBAL_millisecondsUpdateCheckTimeout = 0;
static bool GLOBAL_updateCheckSucceeded = false;
static bool GLOBAL_isMostRecentVersion = false;
static volatile bool GLOBAL_skipPrintsFromThread = false;


// CreateThread function
static DWORD WINAPI versionCheckThread(LPVOID lpParameter) {

    char mostRecentVersionDate[sizeof(thisVersionDate)] = { 0 };

    // WinHttpReadData loop variables
    DWORD bytesReadThisLoop = 0;
    DWORD totalBytesRead = 0;

    HINTERNET hInternet = nullptr;
    HINTERNET hConnection = nullptr;
    HINTERNET hData = nullptr;

    hInternet = WinHttpOpen(            // resource acquired (1)
        L"amnesia_load_screen_tool",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (hInternet == nullptr) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf("Error when using WinHttpOpen (error code %u).\n", GetLastError());
        }

        goto RESOURCE_CLEANUP;
    }

    WinHttpSetTimeouts(
        hInternet,
        GLOBAL_millisecondsUpdateCheckTimeout, // Resolve timeout
        GLOBAL_millisecondsUpdateCheckTimeout, // Connect timeout
        GLOBAL_millisecondsUpdateCheckTimeout, // Send timeout
        GLOBAL_millisecondsUpdateCheckTimeout  // Receive timeout
    );

    hConnection = WinHttpConnect(        // resource acquired (2)
        hInternet,
        L"raw.githubusercontent.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (hConnection == nullptr) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf("Error when using WinHttpConnect (error code %u).\n", GetLastError());
        }

        goto RESOURCE_CLEANUP;
    }

    hData = WinHttpOpenRequest(            // resource acquired (3)
        hConnection,
        L"GET",
        L"/speedrun-program/amnesia_load_screen_tool/main/version_date.txt",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (hData == nullptr) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf("Error when using WinHttpOpenRequest (error code %u).\n", GetLastError());
        }

        goto RESOURCE_CLEANUP;
    }

    if (!WinHttpSendRequest(hData, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf("Error when using WinHttpSendRequest (error code %u).\n", GetLastError());
        }

        goto RESOURCE_CLEANUP;
    }

    if (!WinHttpReceiveResponse(hData, nullptr)) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf("Error when using WinHttpReceiveResponse (error code %u).\n", GetLastError());
        }

        goto RESOURCE_CLEANUP;
    }

    while (
        totalBytesRead < sizeof(mostRecentVersionDate) - 1
        && WinHttpReadData(hData, &mostRecentVersionDate[totalBytesRead], sizeof(mostRecentVersionDate) - 1 - totalBytesRead, &bytesReadThisLoop)
        && bytesReadThisLoop != 0
        ) {

        totalBytesRead += bytesReadThisLoop;

        if (!GLOBAL_skipPrintsFromThread) {
            printf("%u out of %zu date string bytes read (\"%s\").\n", totalBytesRead, sizeof(mostRecentVersionDate) - 1, mostRecentVersionDate);
        }
    }

    if (totalBytesRead < sizeof(mostRecentVersionDate) - 1) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf("Couldn't determine the date of the most recent version of this tool using InternetReadFile (error code %u).\n", GetLastError());
        }

        goto RESOURCE_CLEANUP;
    }

    GLOBAL_updateCheckSucceeded = true;

    GLOBAL_isMostRecentVersion = (strncmp(thisVersionDate, mostRecentVersionDate, sizeof(mostRecentVersionDate) - 1) == 0);

    if (!GLOBAL_isMostRecentVersion) {
        if (!GLOBAL_skipPrintsFromThread) {
            printf(
                "A newer version of this tool released on %s is available on https://www.speedrun.com/tdd/resources.\nThis version's date is %s.\n",
                mostRecentVersionDate,
                thisVersionDate
            );
        }
    }

RESOURCE_CLEANUP:
    if (hData != nullptr) {
        WinHttpCloseHandle(hData);          // resource released (3)
    }
    if (hConnection != nullptr) {
        WinHttpCloseHandle(hConnection);    // resource released (2)
    }
    if (hInternet != nullptr) {
        WinHttpCloseHandle(hInternet);      // resource released (1)
    }

    return 0;
}


static bool isMostRecentVersion(bool* isMostRecentVersionResult, const uint32_t millisecondsUpdateCheckTimeout) {

    GLOBAL_millisecondsUpdateCheckTimeout = (DWORD)millisecondsUpdateCheckTimeout;

    bool updateCheckSucceeded = false;
    *isMostRecentVersionResult = false;

    HANDLE hThread = nullptr;

    hThread = CreateThread(    // resource acquired
        nullptr,
        0,
        versionCheckThread,
        nullptr,
        0,
        nullptr
    );

    if (hThread == nullptr) {
        printf("Error when using CreateThread (error code %u).\n", GetLastError());

        return false;
    }

    DWORD threadResult = WaitForSingleObject(hThread, millisecondsUpdateCheckTimeout);

    if (threadResult == WAIT_OBJECT_0) {

        updateCheckSucceeded = GLOBAL_updateCheckSucceeded;
        *isMostRecentVersionResult = GLOBAL_isMostRecentVersion;

    } else if (threadResult == WAIT_TIMEOUT) {

        GLOBAL_skipPrintsFromThread = true;

        printf("Update check thread timed out.\n");

    } else if (threadResult == WAIT_FAILED) {

        GLOBAL_skipPrintsFromThread = true;

        printf("Update check thread failed with error code %u.\n", GetLastError());

    } else {

        GLOBAL_skipPrintsFromThread = true;

        printf("Update check thread ended with return code %u.\n", threadResult);

    }

    if (hThread != nullptr) {
        CloseHandle(hThread);                                     // resource released
    }

    return updateCheckSucceeded;
}
