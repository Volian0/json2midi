#include "pt2.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);
    try
    {
        std::cout << args.at(0) << std::endl;
        pt2::song song(args);
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception:\n" << e.what() << std::endl;
    }
    std::cin.get();
    return -1;
}
