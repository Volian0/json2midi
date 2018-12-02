#include "song_handler.h"

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.push_back(argv[i]);
    return pt2::handle_file(args);
}
