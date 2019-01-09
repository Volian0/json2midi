#include "song_handler.h"
#include "pt2.h"
#include "csv.h"
#include <iostream>

namespace pt2
{

int handle_song(const std::vector<std::string>& args)
try
{
    std::cout << args.at(0) << std::endl;
    song song(args);
    for (uint32_t p=0; p<song.parts.size(); ++p)
        if (song.parts[p].HasWarnings())
        {
            std::cout << "Part " << p+1 << ":" << std::endl;
            for (uint32_t t=0; t<song.parts[p].tracks.size(); ++t)
                if (!song.parts[p].tracks[t].warnings.empty())
                {
                    std::cout << "  Track " << t+1 << ":" << std::endl;
                    for (const auto& w : song.parts[p].tracks[t].warnings)
                        std::cout << "    " << w << std::endl;
                }
        }
    if (args.empty()||args.back()!="-f")
            std::cin.get();
    return 0;
}
catch (const std::exception& e)
{
    std::cout << "Exception:\n" << e.what() << std::endl;
    if (args.empty()||args.back()!="-f")
        std::cin.get();
    return -1;
}

void handle_csv(const std::string& filename)
{
    std::vector<std::string> args;
    csv::CSVfile csv(filename);
    uint32_t stat_allsongs{};
    uint32_t stat_goodsongs{};
    for (auto i=csv.records.begin(); i<=csv.records.end(); ++i)
    {
        if (!args.empty()&&(i==csv.records.end()||args[0]!=i->fields.at(5)))
        {
            ++stat_allsongs;
            args[0] = args[0] + ".json";
            args.push_back("-f");
            if (handle_song(args)==0)
                ++stat_goodsongs;
            args.clear();
        }
        if (i==csv.records.end())
            break;
        if (i->fields.at(2).empty()||i->fields.at(3).empty())
        {
            args.clear();
        }
        else
        {
            if (args.empty())
                args.push_back(i->fields.at(5));
            args.push_back(i->fields.at(2));
            args.push_back(i->fields.at(3));
        }
    }
    std::cout << "================================" << std::endl;
    std::cout << stat_goodsongs << " of " << stat_allsongs << " songs were converted." << std::endl;
    std::cout << "Conversion rate is:" << std::endl;
    std::cout << static_cast<long double>(stat_goodsongs*100)/static_cast<long double>(stat_allsongs) << "%" << std::endl;
    std::cout << "================================" << std::endl;
    std::cin.get();
}

int handle_file(const std::vector<std::string>& args)
{
    const std::string& name = args.at(0);
    if (name.size()>=4&&name.substr(name.size()-4,4)==".csv")
        handle_csv(name);
    else
        return handle_song(args);
    return 0;
}

}
