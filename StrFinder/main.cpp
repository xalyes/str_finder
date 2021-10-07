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

class CLogReader
{
public:
    CLogReader()
        : m_filter(nullptr)
        , m_filterLength(0)
        , m_file(nullptr)
        , m_buffer(nullptr)
        , m_offset(0)
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
    size_t m_offset;
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
}

bool CLogReader::SetFilter(const char* filter)
{
    size_t len = strlen(filter);

    m_filter = static_cast<char*>(malloc(len + 1));
    if (!m_filter)
        return false;

    m_filterLength = len;

    size_t copied = 0;
    bool star = false;
    for (size_t i = 0; char c = filter[i]; ++i)
    {
        if (c == '*')
        {
            if (star)
            {
                --m_filterLength;
                continue;
            }
            else
            {
                m_filter[copied++] = c;
                star = true;
                continue;
            }
        }
        else
        {
            m_filter[copied++] = c;
            star = false;
        }
    }
    m_filter[copied] = '\0';

    return true;
}

bool CLogReader::GetNextLine(char* buf, const int bufsize)
{
    struct MatchContext
    {
        Vector<size_t> possibleStates;

    } context;

    //const char* mask = "ab*vd?fd*";
    //const char* str = "abacababababaccvdcfd";

    constexpr auto ChunkSize = 4096;
    m_buffer = static_cast<char*>(malloc(ChunkSize));
    if (!m_buffer)
        return false;

    size_t readSize = 0;

    readSize = fread_s(m_buffer, ChunkSize, 1, ChunkSize, m_file);
    if (ferror(m_file))
        false;

    size_t begin = m_offset;

    while (true)
    {
        Vector<size_t> possibleStates;
        if (possibleStates.Initialize(m_filterLength))
            return false;

        if (possibleStates.PushBack(0))
            return false;

        size_t i = begin;
        while (i != readSize)
        {
            char c = m_buffer[i];
            if (c == '\r' || c == '\n')
                break;

            auto statesSize = possibleStates.Size();
            for (size_t k = 0; k < statesSize; ++k)
            {
                char filterChar = m_filter[possibleStates[k]];
                if (c == filterChar || filterChar == '?')
                {
                    possibleStates[k]++;
                }
                else if (filterChar == '*')
                {
                    if (possibleStates.PushBack(possibleStates[k] + 1))
                        return false;
                }
                else
                {
                    possibleStates.Erase(k);
                }
            }
            ++i;
        }
        
        if (i == readSize)
        {
            readSize = 0;

            readSize = fread_s(m_buffer, ChunkSize, ChunkSize, 1, m_file);
            if (ferror(m_file))
                false;

            begin = 0;
            m_offset = 0;
        }

        for (size_t k = 0; k < possibleStates.Size(); ++k)
        {
            if (!m_filter[possibleStates[k]])
            {
                strncpy_s(buf, bufsize, m_buffer, i - begin);
                m_offset = i + 1;
                return true;
            }
        }
        begin = i + 1;
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

    constexpr auto MaxSize = 4096;
    char* result = static_cast<char*>(malloc(MaxSize));
    while (reader.GetNextLine(result, MaxSize))
    {
        std::cout << result << std::endl;
    }

    return 0;
}
