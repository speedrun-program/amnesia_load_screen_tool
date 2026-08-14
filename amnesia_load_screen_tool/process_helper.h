
#pragma once


class ProcessHelper {

public:

    HANDLE processHandle = nullptr;
    uint32_t whereToReadOrWrite = 0;
    uint32_t remainingBytesToRead = 0;
    uint32_t textSegmentLocation = 0;
    DWORD bufferPosition = 0;
    unsigned char buffer[4096] = { 0 };

    ProcessHelper(const ProcessHelper& fhelper) = delete;
    ProcessHelper& operator=(ProcessHelper other) = delete;
    ProcessHelper(ProcessHelper&&) = delete;
    ProcessHelper& operator=(ProcessHelper&&) = delete;


    ProcessHelper(const DWORD pid, const wchar_t* processName) {

        processHandle = OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_SUSPEND_RESUME | PROCESS_TERMINATE,
            false,
            pid
        );

        if (processHandle == nullptr) {
            printf("ProcessHelper couldn't get process handle for %ls with PID %u (error code %u).\n", processName, pid, GetLastError());
            return;
        }

        if (!checkIfProcessIsCorrect(processName)) {
            return;
        }

        if (!findTextSegmentLocation(processName)) {
            return;
        }
    }


    ~ProcessHelper() {

        if (processHandle != nullptr) {
            CloseHandle(processHandle);
        }
    }


    bool checkIfProcessIsCorrect(const wchar_t* processName) const {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        wchar_t filepathBuffer[320] = { 0 };

        DWORD charactersWritten = (sizeof(filepathBuffer) / sizeof(wchar_t)) - 1; // - 1 so there's always a L'\0' at the end
        DWORD queryFullProcessImageNameResult = QueryFullProcessImageName(
            processHandle,
            PROCESS_NAME_NATIVE,
            filepathBuffer,
            &charactersWritten
        );

        if (queryFullProcessImageNameResult == 0) {
            DWORD errorNumber = GetLastError();

            if (errorNumber == 122) {
                printf("The file path of Amnesia.exe or Amnesia_NoSteam.exe was too long to read (error code 122): %ls.\n", filepathBuffer);
            } else {
                printf("Error when using QueryFullProcessImageName (error code %u).\n", errorNumber);
            }

            return false;
        }

        wchar_t* filename = wcsrchr(filepathBuffer, L'\\');
        filename = (filename != nullptr) ? filename + 1 : filepathBuffer;

        if (wcscmp(filename, processName) != 0) {
            printf("Unexpected process name when checking it using QueryFullProcessImageName: %ls.\n", filepathBuffer);
            return false;
        }

        return true;
    }


    bool findTextSegmentLocation(const wchar_t* processName) {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        wchar_t filepathBuffer[320] = { 0 };
        wchar_t filepathBufferCopy[(sizeof(filepathBuffer) / sizeof(wchar_t))] = { 0 };

        uint32_t queryAddress = 0;
        MEMORY_BASIC_INFORMATION mbi = { 0 };

        if (VirtualQueryEx(processHandle, (LPCVOID)queryAddress, &mbi, sizeof(mbi)) == 0) { // checking if VirtualQueryEx works
            printf("Error when using VirtualQueryEx (error code %u).\n", GetLastError());
            return false;
        }

        // finding start of exe memory area
        DWORD charactersWritten = 0;
        bool foundExeArea = false;

        for (queryAddress += mbi.RegionSize; VirtualQueryEx(processHandle, (LPCVOID)queryAddress, &mbi, sizeof(mbi)) != 0; queryAddress += mbi.RegionSize) {
            DWORD charactersWritten = GetMappedFileName(
                processHandle,
                (LPVOID)queryAddress,
                filepathBuffer,
                (sizeof(filepathBuffer) / sizeof(wchar_t)) - 1 // - 1 so there's always a L'\0' at the end
            );

            if (charactersWritten == 0) {
                continue;
            }

            wchar_t* filename = wcsrchr(filepathBuffer, L'\\');
            filename = (filename != nullptr) ? filename + 1 : filepathBuffer;

            if (wcscmp(filename, processName) == 0) {
                foundExeArea = true;
                memcpy(filepathBufferCopy, filepathBuffer, sizeof(filepathBuffer));
                break;
            }
        }

        if (!foundExeArea) {
            printf("Couldn't find the executable's memory area.\n");
            return false;
        }
        
        // finding the .text area
        do {
            if (mbi.Protect == PAGE_EXECUTE_READ) {
                remainingBytesToRead = mbi.RegionSize;
                whereToReadOrWrite = queryAddress;

                if (!refillBuffer()) {
                    return false; // first read failed
                }

                textSegmentLocation = queryAddress; // do this last to indicate successful initialization
                return true;
            }

            charactersWritten = GetMappedFileName(
                processHandle,
                (LPVOID)queryAddress,
                filepathBuffer,
                (sizeof(filepathBuffer) / sizeof(wchar_t)) - 1 // - 1 so there's always a L'\0' at the end
            );

            // no longer looking at exe memory
            if (charactersWritten == 0 || wcscmp(filepathBuffer, filepathBufferCopy) != 0) {
                break;
            }

            queryAddress += mbi.RegionSize;
        } while (VirtualQueryEx(processHandle, (LPCVOID)queryAddress, &mbi, sizeof(mbi)) != 0);

        printf("Couldn't find .text memory area in the executable's memory area.\n");
        return false;
    }


    bool refillBuffer() const {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        uint32_t bytesToRead = (remainingBytesToRead >= sizeof(buffer)) ? sizeof(buffer) : remainingBytesToRead;
        uint32_t bytesReadSoFar = 0;

        while (bytesReadSoFar < bytesToRead) {
            SIZE_T bytesReadOnCurrentCall = 0;

            bool readSucceeded = ReadProcessMemory(
                processHandle,
                (LPCVOID)whereToReadOrWrite,
                (LPVOID)(&buffer[bytesReadSoFar]),
                bytesToRead - bytesReadSoFar,
                &bytesReadOnCurrentCall
            );
            if (bytesReadOnCurrentCall == 0) {
                printf("ReadProcessMemory couldn't read any bytes starting at memory address: 0x%x.\n", whereToReadOrWrite);
                return false;
            }
            if (!readSucceeded) {
                printf("ReadProcessMemory error %u at memory address: 0x%x.\n", GetLastError(), whereToReadOrWrite);
                return false;
            }

            bytesReadSoFar += bytesReadOnCurrentCall;
        }

        return true;
    }


    bool getByte(unsigned char* b) {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        if (remainingBytesToRead == 0) {
            return false;
        }

        if (bufferPosition == sizeof(buffer)) {
            bufferPosition = 0;
            whereToReadOrWrite += sizeof(buffer);

            if (!refillBuffer()) {
                return false;
            }
        }

        *b = buffer[bufferPosition];
        bufferPosition += 1;
        remainingBytesToRead -= 1;

        return true;
    }


    uint32_t writeToProcess(const uint32_t writeLocation, const unsigned char* src, const uint32_t howManyBytesToWrite) const {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        uint32_t totalBytesWritten = 0;

        while (totalBytesWritten < howManyBytesToWrite) {
            SIZE_T bytesWrittenOnCurrentCall = 0;

            bool writeSucceeded = WriteProcessMemory(
                processHandle,
                (LPVOID)writeLocation,
                (LPCVOID)(&src[totalBytesWritten]),
                howManyBytesToWrite - totalBytesWritten,
                &bytesWrittenOnCurrentCall
            );

            if (bytesWrittenOnCurrentCall == 0) {
                printf("WriteProcessMemory couldn't write any bytes starting at memory address: 0x%x.\n", writeLocation);

                if (!writeSucceeded) {
                    printf("WriteProcessMemory error %u.\n", GetLastError());
                }

                return totalBytesWritten;
            }

            if (!writeSucceeded) {
                printf("WriteProcessMemory error %u at memory address: 0x%x.\n", GetLastError(), writeLocation);

                return totalBytesWritten;
            }

            totalBytesWritten += bytesWrittenOnCurrentCall;
        }

        return totalBytesWritten;
    }


    bool writeByte(const unsigned char b) {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        if (bufferPosition == sizeof(buffer)) {
            bufferPosition = 0;

            if (writeToProcess(whereToReadOrWrite, buffer, sizeof(buffer)) != sizeof(buffer)) {
                return false;
            }

            whereToReadOrWrite += sizeof(buffer);
        }

        buffer[bufferPosition] = b;
        bufferPosition += 1;

        return true;
    }


    bool flushBuffer() {

        if (processHandle == nullptr) {
            printf("ProcessHelper error: HANDLE processHandle was NULL.\n");

            return false;
        }

        bool bufferFlushedSuccessfully = writeToProcess(whereToReadOrWrite, buffer, bufferPosition) == bufferPosition;
        bufferPosition = 0;

        return bufferFlushedSuccessfully;
    }
};
