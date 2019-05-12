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
    uint32_t value;
public:
    uint8_t GetType() const; // 0 - note on, 1 - note off, 2 - length, 3 - ignore
    operator uint32_t() const;
    message(uint32_t,uint8_t); // sets value
};

class track
{
protected:
    //std::string instrument; // optional, used for patching
public:
    uint16_t basebeats;
    std::vector<std::string> warnings;
    std::vector<message> messages;
    void parse(const std::string&,long double);
    uint32_t GetLength(const std::string&,bool) const; // 0 - lengths, 1 - rests
    int32_t operator-(const track& tr) const;
    void shrink(uint32_t);
};

class part
{
protected:
public:
    long double bpm; // tempo
    uint16_t basebeats; // multiplier for tempo and tile length
    std::vector<track> tracks;
    void VerifyLength();
    bool HasWarnings() const;
};

class song // this class handles json to midi conversion
{
protected:
    std::vector<std::string> arguments;
    void ParseJSON();
    void VerifyTracks();
    void MakeMIDI(const std::string&);
public:
    std::vector<part> parts;
    song(const std::vector<std::string>&);
};

class safe_divider // class for dividing in a safe way
{
protected:
    uint32_t remainder{};
public:
    uint32_t divide(uint32_t,uint32_t);
};

uint8_t GetNote(const std::string&); // 0 - error, 1 - empty/mute, 2 - ~, 3 - @, 4 - %, 5 - !, 6 - ^

}

#endif
