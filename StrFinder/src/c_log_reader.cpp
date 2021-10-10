#include <malloc.h>
#include <string.h>
#include <new>

#include "c_log_reader.h"

// --------------------------------------------------------------------------------
class ReadContext
{
public:
    ReadContext(size_t chunkSize, FILE* file) noexcept;

    bool CheckedInit() noexcept;
    bool LoadChunkFromFile() noexcept;
    bool EofReached() noexcept;
    ~ReadContext() noexcept;

private:
    const size_t m_chunkSize;
    FILE* m_file;
    bool m_eofReached;

public:
    char* buffer;
    size_t bufferSize;

    size_t currentBufferPosition;
};

// --------------------------------------------------------------------------------
ReadContext::ReadContext(size_t chunkSize, FILE* file) noexcept
    : m_chunkSize(chunkSize)
    , m_file(file)
    , m_eofReached(false)
    , buffer(nullptr)
    , bufferSize(0)
    , currentBufferPosition(0)
{}

// --------------------------------------------------------------------------------
bool ReadContext::CheckedInit() noexcept
{
    if (!buffer)
    {
        buffer = static_cast<char*>(malloc(m_chunkSize));
        if (!buffer)
            return false;
    }
    return true;
}

// --------------------------------------------------------------------------------
bool ReadContext::LoadChunkFromFile() noexcept
{
    bufferSize = fread_s(buffer, m_chunkSize, 1, m_chunkSize, m_file);
    if (ferror(m_file))
        return false;

    if (bufferSize != m_chunkSize)
        m_eofReached = true;

    currentBufferPosition = 0;
    return true;
}

// --------------------------------------------------------------------------------
bool ReadContext::EofReached() noexcept
{
    return m_eofReached;
}

// --------------------------------------------------------------------------------
ReadContext::~ReadContext() noexcept
{
    if (m_file)
        fclose(m_file);

    free(buffer);
}

// --------------------------------------------------------------------------------
class ScopedFree
{
public:
    ScopedFree(void* ptr) noexcept
        : m_ptr(ptr)
    {}

    ~ScopedFree() noexcept
    {
        free(m_ptr);
    }

    ScopedFree(ScopedFree&& other) = delete;
    ScopedFree& operator =(ScopedFree&& other) = delete;
    ScopedFree(const ScopedFree&) = delete;
    ScopedFree& operator =(const ScopedFree&) = delete;

private:
    void* m_ptr;
};

// --------------------------------------------------------------------------------
static bool ExchangeBuffers(size_t bufsize, ReadContext* readContext, char* outputBuffer, size_t& outputBufferWrittenSize, size_t& begin, size_t& end) noexcept
{
    size_t copySize = 0;

    if (end - begin < bufsize)
    {
        copySize = end - begin;
    }
    else
    {
        copySize = bufsize - 1;
    }

    size_t freeSpace = bufsize - outputBufferWrittenSize - 1;
    if (freeSpace < copySize)
        copySize = freeSpace;

    if (copySize > 0)
    {
        if (strncpy_s(outputBuffer + outputBufferWrittenSize, bufsize - outputBufferWrittenSize, readContext->buffer + begin, copySize))
            return false;

        outputBufferWrittenSize += copySize;
    }

    if (!readContext->LoadChunkFromFile())
        return false;

    begin = 0;
    end = 0;
    return true;
}

// --------------------------------------------------------------------------------
static bool FillResult(char* buf, size_t bufsize, ReadContext* readContext, char* outputBuffer, size_t outputBufferWrittenSize, size_t begin, size_t end) noexcept
{
    if (outputBufferWrittenSize)
    {
        if (strncpy_s(buf, bufsize, outputBuffer, outputBufferWrittenSize))
            return false;
    }

    size_t copySize = 0;

    if (end - begin < bufsize)
    {
        copySize = end - begin;
    }
    else
    {
        copySize = bufsize - 1;
    }

    size_t freeSpace = bufsize - outputBufferWrittenSize - 1;
    if (freeSpace < copySize)
        copySize = freeSpace;

    if (copySize > 0)
    {
        if (strncpy_s(buf + outputBufferWrittenSize, bufsize - outputBufferWrittenSize, readContext->buffer + begin, copySize))
            return false;
    }

    readContext->currentBufferPosition = end + 1;
    return true;
}

// --------------------------------------------------------------------------------
bool CLogReader::Open(const char* filePath) noexcept
{
    FILE* file;
    if (fopen_s(&file, filePath, "r"))
        return false;

    m_readContext = static_cast<ReadContext*>(malloc(sizeof(ReadContext)));
    if (!m_readContext)
        return false;

    constexpr auto ChunkSize = 25;
    new (m_readContext) ReadContext(ChunkSize, file);

    return true;
}

// --------------------------------------------------------------------------------
void CLogReader::Close() noexcept
{
    m_readContext->~ReadContext();
    free(m_readContext);

    free(m_filter);
}

// --------------------------------------------------------------------------------
bool CLogReader::SetFilter(const char* filter) noexcept
{
    m_filter = _strdup(filter);
    if (!m_filter)
        return false;

    m_filterLength = strlen(filter);

    return true;
}

// --------------------------------------------------------------------------------
bool CLogReader::GetNextLine(char* buf, const int bufsize) noexcept
{
    // Check end of file
    if (m_readContext->EofReached())
        return false;

    // Init read context if need
    if (!m_readContext->CheckedInit())
        return false;

    // Load the next chunk from file if need
    if (m_readContext->bufferSize == m_readContext->currentBufferPosition)
    {
        if (!m_readContext->LoadChunkFromFile())
            return false;
    }

    // Allocate output buffer
    char* outputBuffer = static_cast<char*>(malloc(bufsize));
    if (!outputBuffer)
        return false;

    ScopedFree autoFreePtr(outputBuffer);

    size_t outputBufferWrittenSize = 0;
    size_t begin = m_readContext->currentBufferPosition;

    // Handy wrappers
    auto exchangeBuffers = [&](size_t& i)
    {
        return ExchangeBuffers(bufsize, m_readContext, outputBuffer, outputBufferWrittenSize, begin, i);
    };
    auto fillResult = [&](size_t i)
    {
        return FillResult(buf, bufsize, m_readContext, outputBuffer, outputBufferWrittenSize, begin, i);
    };

    while (true)
    {
        size_t i = begin;
        size_t filterPos = 0;
        long lastStarPos = -1;
        bool skipMode = false;

        while (filterPos < m_filterLength)
        {
            char c = m_readContext->buffer[i];
            if (c == '\n')
            {
                skipMode = false;
                break;
            }

            if (!skipMode)
            {
                if (m_filter[filterPos] == '*')
                {
                    lastStarPos = filterPos++;
                    continue;
                }

                if ((c == m_filter[filterPos]) || ('?' == m_filter[filterPos]))
                {
                    filterPos++;
                }
                else
                {
                    if (lastStarPos != -1)
                    {
                        filterPos = lastStarPos + 1;
                    }
                    else
                    {
                        skipMode = true;
                        continue;
                    }
                }
            }
            ++i;

            if (i >= m_readContext->bufferSize)
            {
                if (m_readContext->EofReached())
                    break;

                if (!exchangeBuffers(i))
                    return false;
            }
        }

        if (m_readContext->buffer[i] == '\n' || m_readContext->EofReached())
        {
            // Matching are ok if we passed through the all chars of filter or if the last char is '*'
            if ((filterPos == m_filterLength) || (filterPos == m_filterLength - 1 && m_filter[filterPos] == '*'))
                return fillResult(i);

            if (m_readContext->EofReached())
                return false;

            outputBufferWrittenSize = 0;
            begin = i + 1;
        }
        else
        {
            if (m_filter[filterPos - 1] == '*')
            {
                // skip chars to EOL or EOF
                while (m_readContext->buffer[i] != '\n')
                {
                    if (i == m_readContext->bufferSize)
                    {
                        if (m_readContext->EofReached())
                            break;

                        if (!exchangeBuffers(i))
                            return false;
                    }
                    else
                    {
                        ++i;
                    }
                }

                return fillResult(i);
            }
            else
            {
                begin = i + 1;
            }
        }
    }

    return false;
}
