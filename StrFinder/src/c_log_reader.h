#pragma once

#include <stdio.h>

class CLogReader
{
public:
    CLogReader()
        : m_filter(nullptr)
        , m_filterLength(0)
        , m_file(nullptr)
        , m_buffer(nullptr)
        , m_bufferSize(0)
        , m_bufferAllocated(false)
        , m_readSize(0)
        , eofReached(false)
    {}

    ~CLogReader() = default;

    bool Open(const char* filePath);
    void Close();

    bool SetFilter(const char* filter);
    bool GetNextLine(char* buf, const int bufsize);

private:
    char* m_filter;
    size_t m_filterLength;

    FILE* m_file;

    char* m_buffer;
    size_t m_bufferSize;
    bool m_bufferAllocated;

    size_t m_readSize;
    bool eofReached;
};
