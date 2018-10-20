#ifndef PT2_H
#define PT2_H

#include <cstdint>
#include <vector>
#include <string>

namespace pt2
{

/* This is a class for storing
   Note messages and lengths   */
class message
{
protected:
    const uint32_t value;
public:
    uint8_t GetType() const;
    operator uint32_t() const;
    message(uint32_t); // sets value
};

class track : public std::vector<message>
{
protected:
    std::string instrument; // optional, used for patching
};

class part : public std::vector<track>
{
protected:
    const uint32_t bpm; // tempo
public:
    part(uint32_t); // sets bpm (tempo)
};

class song : protected std::vector<part>
{
protected:
    void ParseJSON(const std::string&);
public:
    song(const std::string&);
};

}

#endif
