#include <iostream>

#include "src/c_log_reader.h"

#define TEST

#ifdef TEST

#include <vector>
#include <string>
#include <map>

std::map<std::string, std::vector<std::string>> TestCases =
{
  { "*est laborum.",
    {
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "officia deserunt mollit anim id est laborum.",
        "officia deserunt mollit anim id est laborum.",
        "officia deserunt mollit anim id est laborum.",
        "officia deserunt mollit anim id est laborum.",
        "officia deserunt mollit anim id est laborum.",
        "officia deserunt mollit anim id est laborum.",
        "officia deserunt mollit anim id est laborum.",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "officia deserunt mollit anim id est laborum.",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "officia deserunt mollit anim id est laborum.",
        "Lorem ipsum dolor sit amet, consectetur adipiscin"
    } },

  { "*sorting*",
    {
        "A large part of the C++ library is based on the S",
        "Our first example is a problem called topological",
        "The problem of topological sorting is to embed th",
        "As an example of topological sorting, imagine a l",
        "Fig. 7. The ordering relation of Fig. 6 after top",
        "topological sorting in this case is to find a way",
        "There is a very simple way to do topological sort",
        "Fig. 9. Topological sorting.",
        "A topological sorting technique similar to Algori",
        "(1962), 558-562. The fact that topological sortin",
        "better algorithm for topological sorting in Secti",
        "\"\"' 11. [ 24] The result of topological sorting i"
    } },

  { "Lorem ipsum*",
    {
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "Lorem ipsum dolor sit amet, consectetur adipiscin"
    } },

  { "*ullamco*",
    {
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "Lorem ipsum dolor sit amet, consectetur adipiscin",
        "ullamco laboris nisi ut aliquip ex ea commodo con",
        "Lorem ipsum dolor sit amet, consectetur adipiscin"
    } }
};

int main()
{
    for (auto& testCase : TestCases)
    {
        CLogReader reader;
        if (!reader.Open("TestData\\169Kb\\169Kb_text.txt"))
            return -2;

        if (!reader.SetFilter(testCase.first.c_str()))
            return -2;

        constexpr auto MaxSize = 50;
        char* result = static_cast<char*>(malloc(MaxSize));
        if (!result)
            return -1;

        for (auto& expectedStr : testCase.second)
        {
            if (!reader.GetNextLine(result, MaxSize))
            {
                std::cout << "Expected string '" << expectedStr << "' but false is returned.\n";
                break;
            }

            std::string actual(result);
            if (actual != expectedStr)
                std::cout << "Expected string '" << expectedStr << "' but string '" << actual << "' is returned.\n";
        }

        if (reader.GetNextLine(result, MaxSize))
            std::cout << "Expected false returning but true is returned.\n";

        free(result);
        reader.Close();
    }

    std::cout << "Tests finished\n";
    return 0;
}

#else

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

#endif
