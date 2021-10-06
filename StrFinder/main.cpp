#include <malloc.h>
#include <errno.h>

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
    CLogReader() {};
    ~CLogReader() {};

    bool Open();
    void Close();

    bool SetFilter(const char* filter);
    bool GetNextLine(char* buf, const int bufsize);
};

bool CLogReader::SetFilter(const char* filter)
{
    char states[20] = { 0 };
    bool stars[20] = { 0 };
    int f = 0;
    {
    size_t i = 0;
    size_t j = 0;
    bool star = false;
    while (filter[i])
    {
        if (filter[i] == '*')
        {
            star = true;
            ++i;
            continue;
        }

        states[j] = filter[i];
        if (star)
        {
            stars[j] = true;
        }
        else
        {
            stars[j] = false;
        }

        ++i;
        ++j;
        star = false;
    }
    f = j;
    }

    auto move = [&states, &stars](int s[20], char c)
    {
        int res[20] = { -1 };
        size_t i = 0;
        size_t j = 0;
        while (s[i] != -1)
        {
            if (stars[i])
            {
                res[j] = s[i];
                j++;
            }
            else if (c == states[i] || '?' == states[i])
            {
                res[j] = states[i+1];
                j++;
            }

            i++;
        }

        
        for (size_t i = 0; i < 20; ++i)
        {
            s[i] = res[i];
        }
    };

    const char* str = "aacababababaccvddcfd";

    int currentStates[20] = { -1 };
    currentStates[0] = states[0];

    size_t i = 0;
    while (str[i])
    {
        move(currentStates, str[i]);
        ++i;
    }

    for (size_t i = 0; i < 20; ++i)
    {
        if (i == f)
            return str;
    }
    return true;
}

bool CLogReader::GetNextLine(char* buf, const int bufsize)
{
    const char* mask = "ab*vd?fd*?";

    const char* str = "sabacababababaccvdcfdscs";

    Vector<size_t> possibleStates;
    if (possibleStates.Initialize(50))
        return false;

    if (possibleStates.PushBack(0))
        return false;

    size_t i = 0;
    bool star = false;
    while (char c = str[i])
    {
        auto statesSize = possibleStates.Size();
        for (size_t k = 0; k < statesSize; ++k)
        {
            char filterChar = mask[possibleStates[k]];
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

    for (size_t k = 0; k < possibleStates.Size(); ++k)
    {
        if (!mask[possibleStates[k]])
            return true;
    }

    return false;
}

int main()
{
    CLogReader reader;
    std::cout << reader.GetNextLine(nullptr, 0) << std::endl;
    return 0;
}
