#include "pt2.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);
    if (args.empty())
    {
        std::cout << "Please specify JSON file!\nUsage example:\njson2midi <json> [auto]\njson2midi <json> [bpm1] [bb1] [bpm2] [bb2] [bpm3] [bb3]" << std::endl;
        std::cin.get();
    }
    else
        pt2::song song(args);
    return 0;
}
