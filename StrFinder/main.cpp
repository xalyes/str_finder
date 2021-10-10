#include <malloc.h>
#include <errno.h>
#include <stdio.h>

#include <iostream>

template<class T>
class Stack
{
public:
    Stack() = default;

    int Initialize(size_t depth)
    {
        T* m_stack = static_cast<T*>(malloc(depth * sizeof(T)));

        if (!m_stack)
            return -1;

        m_count = 0;
        m_depth = depth;

        return 0;
    }

    int Destroy()
    {
        for (size_t i = 0; i < m_count; ++i)
        {
            m_stack[i].~T();
        }

        free(m_stack);
        return errno();
    }

    int Push(const T& elem)
    {
        if (m_count == m_depth)
            return -1;

        m_stack[m_depth - m_count] = elem;
        ++m_count;

        return 0;
    }

    int Pop()
    {
        if (!m_count)
            return -1;

        m_stack[m_depth - m_count].~T();
        --m_count;

        return 0;
    }

    int Top(T& elem)
    {
        if (!m_count)
            return -1;

        elem = m_stack[m_depth - m_count];
        return 0;
    }

private:
    T* m_stack;
    size_t m_count;
    size_t m_depth;
};

template<class T>
class Vector
{
public:
    int Initialize(size_t capacity)
    {
        if (m_inited)
            return -1;

        m_vector = static_cast<T*>(malloc(capacity * sizeof(T)));

        if (!m_vector)
            return -1;

        m_size = 0;
        m_capacity = capacity;
        m_inited = true;

        return 0;
    }

    int Destroy()
    {
        if (!m_inited)
            return -1;

        for (size_t i = 0; i < m_size; ++i)
        {
            m_vector[i].~T();
        }

        free(m_vector);
        return errno;
    }

    int PushBack(const T& elem)
    {
        if (m_capacity == m_size)
            return -1;

        m_vector[m_size++] = elem;

        return 0;
    }

    int Erase(size_t index)
    {
        if (index + 1 > m_size)
            return -1;

        m_vector[index].~T();
        m_size--;

        for (size_t i = index; i < m_size; ++i)
        {
            m_vector[i] = m_vector[i + 1];
        }

        return 0;
    }

    size_t Size() const
    {
        return m_size;
    }

    T& operator[](size_t index)
    {
        return m_vector[index];
    }

private:
    T* m_vector;
    size_t m_size;
    size_t m_capacity;
    bool m_inited{ false };
};

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
        , m_offset(0)
        , m_readSize(0)
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

    size_t m_offset;
    size_t m_readSize;
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

        if (!m_bufferSize)
            return false;

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
            if (i >= m_bufferSize)
            {
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

                if (!m_bufferSize)
                {
                    return false;
                }

                m_readSize = 0;
                begin = 0;
                i = 0;
            }

            char c = m_buffer[i];
            if (c == '\n')
            {
                skipMode = false;
                break;
            }

            if (skipMode)
            {
                ++i;
                continue;
            }

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
            ++i;
        }

        if (m_buffer[i] == '\n')
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

                        if (!m_bufferSize)
                        {
                            return false;
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

int main(int argc, char* argv[])
{
    if (argc != 3)
        return -1;

    CLogReader reader;
    if (!reader.Open(argv[1]))
        return -2;

    if (!reader.SetFilter(argv[2]))
        return -2;

    constexpr auto MaxSize = 50;
    char* result = static_cast<char*>(malloc(MaxSize));
    while (reader.GetNextLine(result, MaxSize))
    {
        std::cout << result << std::endl;
    }

    free(result);
    reader.Close();

    return 0;
}
