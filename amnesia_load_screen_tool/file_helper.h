
#pragma once


class FileHelper {

public:

    FILE* m_file = nullptr;
    size_t m_bufferPosition = 0;
    size_t m_charactersRead = 0;
    unsigned char m_buffer[4096] = { 0 };

    FileHelper(const FileHelper& fhelper) = delete;
    FileHelper& operator=(FileHelper other) = delete;
    FileHelper(FileHelper&&) = delete;
    FileHelper& operator=(FileHelper&&) = delete;


    FileHelper(const char* fileName) {

#ifdef _WIN32
        if (fopen_s(&m_file, fileName, "rb") != 0 || m_file == nullptr) {
#else
        if (!(m_file = fopen(fileName, "rb"))) {
#endif
            printf("fopen error when trying to open %s (errno %d).\n", fileName, errno);

            return;
        }
    }


    ~FileHelper() {

        if (m_file != nullptr) {
            fclose(m_file);
        }
    }


    bool getCharacter(char* ch) {

        if (m_file == nullptr) {
            printf("FileHelper error: FILE* m_file was NULL.\n");

            return false;
        }

        if (m_bufferPosition == m_charactersRead) {
            m_bufferPosition = 0;
            m_charactersRead = fread(m_buffer, 1, sizeof(m_buffer), m_file);

            if (m_charactersRead == 0) {
                return false;
            }
        }

        *ch = m_buffer[m_bufferPosition];
        m_bufferPosition += 1;

        return true;
    }
};
