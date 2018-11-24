#ifndef JSON_H
#define JSON_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

namespace json
{

class JsonValue;

struct pair
{
    std::string str;
    JsonValue * value = nullptr;
    ~pair();
};
class JsonObject
{
    friend class JsonFile;
    friend class JsonValue;
    std::vector<pair*> pairs;
    void Add(const std::string & str)
    {
        for (size_t i=0; i<pairs.size(); ++i)
        {
            if (pairs[i]->str==str)
                throw std::exception();
        }
        pairs.push_back(new pair);
        pairs.back()->str=str;
    }
    ~JsonObject()
    {
        for (size_t i=0; i<pairs.size(); ++i)
            delete pairs[i];
    }
public:
    JsonValue * Find(const std::string & str,const bool validate = true)
    {
        for (size_t i=0; i<pairs.size(); ++i)
        {
            if (pairs[i]->str==str)
                return pairs[i]->value;
        }
        if (validate)
            throw std::exception();
        return nullptr;
    }
};
class JsonArray
{
    friend class JsonFile;
    friend class JsonValue;
    std::vector<JsonValue*> values;
    ~JsonArray();
public:
    size_t GetSize()
    {
        return values.size();
    }
    JsonValue * GetValue(const size_t & n)
    {
        if (n<values.size())
            return values[n];
        throw std::exception();
        return nullptr;
    }
};
class JsonValue
{
    friend class JsonFile;
    friend pair;
    friend JsonArray;
    void * value = nullptr;
    uint8_t type=0;
    ~JsonValue()
    {
        if (type==1||type==4)
            delete (std::string *) value;
        else if (type==2)
            delete (JsonObject *) value;
        else if (type==3)
            delete (JsonArray*) value;
    }
public:
    std::string GetString(bool other = false)
    {
        if ((type==1&&!other)||(type==4&&other))
            return *(std::string *) value;
        throw std::exception();
        return nullptr;
    }
    JsonObject * GetObject()
    {
        if (type==2)
            return (JsonObject *) value;
        throw std::exception();
        return nullptr;
    }
    JsonArray * GetArray()
    {
        if (type==3)
            return (JsonArray*) value;
        throw std::exception();
        return nullptr;
    }
    long long GetInteger()
    {
        if (type==4)
            try
            {
                return stoll(*(std::string *) value);
            }
            catch (const std::exception&)
            {
                throw std::exception();
            }
        return 0;
    }
    long double GetFloat()
    {
        if (type==4)
            try
            {
                return stold(*(std::string *) value);
            }
            catch (const std::exception&)
            {
                throw std::exception();
            }
        return 0;
    }
};
pair::~pair()
{
    delete value;
}
JsonArray::~JsonArray()
{
    for (size_t i=0; i<values.size(); ++i)
        delete values[i];
}
class JsonFile
{
    void IgnoreWhiteSpace(const std::string & str,size_t & i)
    {
        while (i<str.length()&&(str[i]==0x20||str[i]==0x09||str[i]==0x0A||str[i]==0x0D))
            ++i;
    }
    std::string GetStringPart(const std::string & str,size_t & i,const size_t size)
    {
        if (i+size>str.length())
            throw std::exception();
        size_t pos = i;
        i=i+size;
        return str.substr(pos,size);
    }
    uint8_t ConvertHex(const char c)
    {
        if (c>='0'&&c<='9')
            return c - '0';
        if (c>='A'&&c<='F')
            return c - 'A' + 10;
        if (c>='a'&&c<='f')
            return c - 'a' + 10;
        throw std::exception();
        return 0;
    }
    std::string GetUnicode(const uint16_t code)
    {
        std::string s;
        if (code <= 0x7F)
            s += (code & 0x7F);
        else if (code <= 0x7FF)
        {
            s += 0xC0 | ((code >> 6) & 0x1F);
            s += 0x80 | (code & 0x3F);
        }
        //else if (code <= 0xFFFF)
        else
        {
            s += 0xE0 | ((code >> 12) & 0xF);
            s += 0x80 | ((code >> 6) & 0x3F);
            s += 0x80 | (code & 0x3F);
        }
        return s;
    }
    std::string ConvertEscape(const std::string & str,size_t & i)
    {
        std::string s = GetStringPart(str,i,1);
        if (s[0]=='\"'||s[0]=='\\'||s[0]=='/')
            return s;
        if (s[0]=='b')
            return "\b";
        if (s[0]=='f')
            return "\f";
        if (s[0]=='n')
            return "\n";
        if (s[0]=='r')
            return "\r";
        if (s[0]=='t')
            return "\t";
        if (s[0]=='u')
        {
            s = GetStringPart(str,i,4);
            return GetUnicode((ConvertHex(s[0])<<12)|(ConvertHex(s[1])<<8)|(ConvertHex(s[2])<<4)|ConvertHex(s[3]));
        }
        throw std::exception();
        return std::string();
    }
    std::string ParseString(const std::string & str,size_t & i)
    {
        std::string s;
        while (true)
        {
            char c=GetStringPart(str,i,1)[0];
            if (c>=0x0000&&c<=0x001F)
                throw std::exception();
            if (c=='\"')
                break;
            if (c=='\\')
                s += ConvertEscape(str,i);
            else
                s += c;
        }
        return s;
    }
    void ValidateOther(const std::string & str)
    {
        if (str=="true"||str=="false"||str=="null")
            return;
        uint8_t mode=0;
        bool allow=false;
        for (size_t i=0; i<str.length(); ++i)
        {
            if (mode==0&&str[i]=='-')
            {
                mode=1;
                continue;
            }
            else if (mode==0)
                mode=1;
            if (mode==1&&str[i]>='1'&&str[i]<='9')
            {
                mode=2;
                continue;
            }
            else if (mode==1&&str[i]=='0')
            {
                mode=3;
                continue;
            }
            else if (mode==1)
                throw std::exception();
            if (mode==2&&!(str[i]>='0'&&str[i]<='9'))
                mode=3;
            if (mode==3&&str[i]=='.')
            {
                mode=4;
                continue;
            }
            else if (mode==3)
                mode=5;
            if (mode==4&&str[i]>='0'&&str[i]<='9')
                allow=true;
            else if (mode==4&&allow)
            {
                allow=false;
                mode=5;
            }
            else if (mode==4)
                throw std::exception();
            if (mode==5&&(str[i]=='e'||str[i]=='E'))
            {
                mode=6;
                continue;
            }
            else if (mode==5)
                throw std::exception();
            if (mode==6&&(str[i]=='+'||str[i]=='-'))
            {
                mode=7;
                continue;
            }
            else if (mode==6)
                mode=7;
            if (mode==7&&str[i]>='0'&&str[i]<='9')
                allow=true;
            else if (mode==7)
                throw std::exception();
        }
        if (mode==2||mode==3||(mode==4&&allow)||mode==5||(mode==7&&allow))
            return;
        throw std::exception();
    }
    JsonObject * ParseObject(const std::string & str,size_t & i)
    {
        JsonObject * object = new JsonObject;
        IgnoreWhiteSpace(str,i);
        char c;
        bool once=true;
        while (true)
        {
            IgnoreWhiteSpace(str,i);
            c = GetStringPart(str,i,1)[0];
            if (c=='}'&&once)
                break;
            once = false;
            if (c!='\"')
                throw std::exception();
            object->Add(ParseString(str,i));
            IgnoreWhiteSpace(str,i);
            if (GetStringPart(str,i,1)[0]!=':')
                throw std::exception();
            object->pairs.back()->value = ParseValue(str,i);
            c = GetStringPart(str,i,1)[0];
            if (c=='}')
                break;
            if (c!=',')
                throw std::exception();
        }
        return object;
    }
    JsonArray * ParseArray(const std::string & str,size_t & i)
    {
        JsonArray * array = new JsonArray;
        IgnoreWhiteSpace(str,i);
        char c;
        bool once=true;
        while (true)
        {
            IgnoreWhiteSpace(str,i);
            c = GetStringPart(str,i,1)[0];
            if (c==']'&&once)
                break;
            once = false;
            array->values.push_back(ParseValue(str,--i));
            c = GetStringPart(str,i,1)[0];
            if (c==']')
                break;
            if (c!=',')
                throw std::exception();
        }
        return array;
    }
    JsonValue * ParseValue(const std::string & str,size_t & i)
    {
        JsonValue * value = new JsonValue;
        IgnoreWhiteSpace(str,i);
        std::string s = GetStringPart(str,i,1);
        if (s[0]=='\"')
        {
            value->value = new std::string(ParseString(str,i));
            value->type = 1;
        }
        else if (s[0]=='{')
        {
            value->value = ParseObject(str,i);
            value->type = 2;
        }
        else if (s[0]=='[')
        {
            value->value = ParseArray(str,i);
            value->type = 3;
        }
        else
        {
            while(i<str.length()&&((str[i]>='0'&&str[i]<='9')||(str[i]>='a'&&str[i]<='z')||str[i]=='.'||str[i]=='+'||str[i]=='-'||str[i]=='E'))
            {
                s+=str[i];
                ++i;
            }
            ValidateOther(s);
            value->value = new std::string(s);
            value->type = 4;
        }
        IgnoreWhiteSpace(str,i);
        return value;
    }
public:
    JsonValue * value = nullptr;
    JsonFile(const std::string & filename)
    {
        delete value;
        std::ifstream file(filename,std::ios::binary);
        if (file)
        {
            std::string str;
            char c;
            size_t i = 0;
            while (file >> std::noskipws >> c)
                str+=c;
            file.close();
            value = ParseValue(str,i);
            if (i!=str.size())
                throw std::exception();
        }
        else
            throw std::exception();
    }
    ~JsonFile()
    {
        delete value;
        value = nullptr;
    }
};

}

#endif
