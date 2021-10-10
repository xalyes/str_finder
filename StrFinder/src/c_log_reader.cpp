#include <malloc.h>
#include <string.h>

#include "c_log_reader.h"

class AutoFreePtr
{
public:
    AutoFreePtr(void* ptr)
        : m_ptr(ptr)
    {}

    ~AutoFreePtr()
    {
        free(m_ptr);
    }

    AutoFreePtr(AutoFreePtr&& other) noexcept
        : m_ptr(other.m_ptr)
    {}

    AutoFreePtr& operator =(AutoFreePtr&& other) noexcept
    {
        free(m_ptr);
        m_ptr = other.m_ptr;
        return *this;
    }

    AutoFreePtr(const AutoFreePtr&) = delete;
    AutoFreePtr& operator =(const AutoFreePtr&) = delete;

private:
    void* m_ptr;

};

bool CLogReader::Open(const char* filePath)
{
    if (fopen_s(&m_file, filePath, "r"))
        return false;

    return true;
}

void CLogReader::Close()
{
    fclose(m_file);
    free(m_buffer);
    free(m_filter);
}

bool CLogReader::SetFilter(const char* filter)
{
    m_filter = _strdup(filter);
    if (!m_filter)
        return false;

    m_filterLength = strlen(filter);

    return true;
}

bool CLogReader::GetNextLine(char* buf, const int bufsize)
{
    constexpr auto ChunkSize = 25;

    if (eofReached)
        return false;

    if (!m_bufferAllocated)
    {
        m_buffer = static_cast<char*>(malloc(ChunkSize));
        if (!m_buffer)
            return false;
        m_bufferAllocated = true;
    }

    if (m_bufferSize == m_readSize)
    {
        m_bufferSize = fread_s(m_buffer, ChunkSize, 1, ChunkSize, m_file);
        if (ferror(m_file))
            false;

        if (m_bufferSize != ChunkSize)
        {
            eofReached = true;
            return false;
        }

        m_readSize = 0;
    }

    size_t outputBufferWrittenSize = 0;
    char* outputBuffer = static_cast<char*>(malloc(bufsize));
    if (!outputBuffer)
        return false;

    AutoFreePtr autoFreePtr(outputBuffer);

    size_t begin = m_readSize;

    while (true)
    {
        size_t i = begin;
        size_t j = 0;
        long lastStarPos = -1;
        bool skipMode = false;

        while (j < m_filterLength)
        {
            char c = m_buffer[i];
            if (c == '\n')
            {
                skipMode = false;
                break;
            }

            if (!skipMode)
            {
                if (m_filter[j] == '*')
                {
                    lastStarPos = j++;
                    continue;
                }

                if ((c == m_filter[j]) || ('?' == m_filter[j]))
                {
                    j++;
                }
                else
                {
                    if (lastStarPos != -1)
                    {
                        j = lastStarPos + 1;
                    }
                    else
                    {
                        skipMode = true;
                        continue;
                    }
                }
            }
            ++i;

            if (i >= m_bufferSize)
            {
                if (eofReached)
                    break;

                size_t copySize = 0;

                if (i - begin < bufsize)
                {
                    copySize = i - begin;
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
                    strncpy_s(outputBuffer + outputBufferWrittenSize, bufsize - outputBufferWrittenSize, m_buffer + begin, copySize);
                    outputBufferWrittenSize += copySize;
                }

                m_bufferSize = fread_s(m_buffer, ChunkSize, 1, ChunkSize, m_file);
                if (ferror(m_file))
                {
                    return false;
                }

                if (m_bufferSize != ChunkSize)
                {
                    eofReached = true;
                }

                m_readSize = 0;
                begin = 0;
                i = 0;
            }
        }

        if (m_buffer[i] == '\n' || eofReached)
        {
            if ((j == m_filterLength) || (j == m_filterLength - 1 && m_filter[j] == '*'))
            {
                if (outputBufferWrittenSize)
                {
                    strncpy_s(buf, bufsize, outputBuffer, outputBufferWrittenSize);
                }

                size_t copySize = 0;

                if (i - begin < bufsize)
                {
                    copySize = i - begin;
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
                    strncpy_s(buf + outputBufferWrittenSize, bufsize - outputBufferWrittenSize, m_buffer + begin, copySize);
                }

                m_readSize = i + 1;
                return true;
            }

            if (eofReached)
                return false;

            outputBufferWrittenSize = 0;
            begin = i + 1;
        }
        else
        {
            if (m_filter[j - 1] != '*')
            {
                begin = i + 1;
            }
            else
            {
                // skip chars to eol
                while (m_buffer[i] != '\n')
                {
                    if (i == m_bufferSize)
                    {
                        if (eofReached)
                            break;

                        size_t copySize = 0;

                        if (i - begin < bufsize)
                        {
                            copySize = i - begin;
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
                            strncpy_s(outputBuffer + outputBufferWrittenSize, bufsize - outputBufferWrittenSize, m_buffer + begin, copySize);
                            outputBufferWrittenSize += copySize;
                        }

                        m_bufferSize = fread_s(m_buffer, ChunkSize, 1, ChunkSize, m_file);
                        if (ferror(m_file))
                        {
                            return false;
                        }

                        if (m_bufferSize != ChunkSize)
                        {
                            eofReached = true;
                        }

                        m_readSize = 0;
                        begin = 0;
                        i = 0;
                    }
                    else
                    {
                        ++i;
                    }
                }

                if (outputBufferWrittenSize)
                {
                    strncpy_s(buf, bufsize, outputBuffer, outputBufferWrittenSize);
                }

                size_t copySize = 0;

                if (i - begin < bufsize)
                {
                    copySize = i - begin;
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
                    strncpy_s(buf + outputBufferWrittenSize, bufsize - outputBufferWrittenSize, m_buffer + begin, copySize);
                }
                m_readSize = i + 1;
                return true;
            }
        }
    }

    return false;
}