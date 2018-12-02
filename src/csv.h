#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <stdexcept>

namespace csv
{

class CSVrecord
{
public:
    std::vector<std::string> fields;
};

class CSVfile
{
public:
    std::vector<CSVrecord> records;
    void parse(const std::string& filename)
    {
        records.clear();
        std::vector<uint8_t> bytes;
        {
            std::ifstream file(filename,std::ios::binary);
            if (!file)
                throw std::runtime_error("Can't open \""+filename+"\"");
            uint8_t c;
            while (file >> std::noskipws >> c)
                bytes.push_back(c);
        }
        if (bytes.empty())
            throw std::runtime_error("Empty CSV");
        records.push_back(CSVrecord());
        records.back().fields.push_back(std::string());
        uint8_t mode{};
        bool req_LF{};
        for (std::vector<uint8_t>::size_type i=0; i<bytes.size(); ++i)
        {
            if (req_LF&&bytes[i]!=0x0A)
            {
                throw std::runtime_error("Expected LF");
            }
            else if (bytes[i]==0x0A)
            {
                req_LF = false;
                if (i!=0&&bytes[i-1]==0x0D)
                {
                    records.back().fields.back() = records.back().fields.back().substr(0,records.back().fields.back().size()-1);
                    if (mode==2)
                        records.back().fields.back()+="\u000D\u000A";
                    else
                    {
                        mode=0;
                        records.push_back(CSVrecord());
                        records.back().fields.push_back(std::string());
                    }
                }
                else
                    records.back().fields.back()+="\u000A";
            }
            else if (bytes[i]==',')
            {
                if (mode==2)
                    records.back().fields.back()+=',';
                else
                {
                    mode=0;
                    records.back().fields.push_back(std::string());
                }
            }
            else if (bytes[i]=='"')
            {
                if (mode==0)
                {
                    mode=2;
                }
                else if (mode==1)
                {
                    mode=2;
                    records.back().fields.back()+='"';
                }
                else if (mode==2)
                {
                    mode=1;
                }
            }
            else
            {
                if (mode==1&&bytes[i]!=0x0D)
                    throw std::runtime_error("Unexpected char");
                else if (mode==1)
                {
                    req_LF = true;
                }
                records.back().fields.back()+=bytes[i];
            }
        }
        if (mode==2)
            throw std::runtime_error("Unexpected EOF");
        if (records.back().fields.size()==1&&records.back().fields.back().empty()&&bytes.back()==0x0A)
            records.pop_back();
        for (const auto& r : records)
        {
            if (r.fields.size()!=records[0].fields.size())
                throw std::runtime_error("Invalid CSV");
        }

    }
    CSVfile(const std::string& filename)
    {
        parse(filename);
    }
};

}
