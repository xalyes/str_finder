#pragma once

#include <stdio.h>

class ReadContext;

class CLogReader
{
public:
    CLogReader() noexcept
        : m_filter(nullptr)
        , m_filterLength(0)
        , m_readContext(nullptr)
    {}

    ~CLogReader() = default;

    bool Open(const char* filePath) noexcept;
    void Close() noexcept;

    bool SetFilter(const char* filter) noexcept;
    bool GetNextLine(char* buf, const int bufsize) noexcept;

private:
    char* m_filter;
    size_t m_filterLength;
    ReadContext* m_readContext;
};
