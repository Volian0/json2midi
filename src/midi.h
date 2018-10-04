#ifndef MIDI_H
#define MIDI_H

#include <vector>   // For std::vector<>
#include <fstream>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <iterator>
#include <algorithm>

namespace midi
{

/* First define a custom wrapper over std::vector<byte>
 * so we can quickly push_back multiple bytes with a single call.
 */
class MIDIvec: public std::vector<uint8_t>
{
public:
    // Methods for appending raw data into the vector:
    template<typename... Args>
    void AddBytes(uint8_t data, Args...args)
    {
        push_back(data);
        AddBytes(args...);
    }
    template<typename... Args>
    void AddBytes(const std::string& s, Args...args)
    {
        insert(end(), s.begin(), s.end());
        AddBytes(args...);
    }
    void AddBytes() { }
};

/* Define a class which encodes MIDI events into a track */
class MIDItrack: public MIDIvec
{
protected:
    uint32_t delay, running_status;
public:
    MIDItrack()
        : MIDIvec(), delay(0), running_status(0)
    {
    }

    // Methods for indicating how much time elapses:
    void AddDelay(uint32_t amount)
    {
        delay += amount;
        if((delay|amount)>0xFFFFFFF) throw std::overflow_error("Time overflow");
    }

    void AddVarLen(uint32_t t)
    {
        if(t >> 21) AddBytes(0x80 | ((t >> 21) & 0x7F));
        if(t >> 14) AddBytes(0x80 | ((t >> 14) & 0x7F));
        if(t >>  7) AddBytes(0x80 | ((t >>  7) & 0x7F));
        AddBytes(((t >> 0) & 0x7F));
    }

    void Flush()
    {
        AddVarLen(delay);
        delay = 0;
    }

    // Methods for appending events into the track:
    template<typename... Args>
    void AddEvent(uint8_t data, Args...args)
    {
        /* MIDI tracks have the following structure:
         *
         * { timestamp [metaevent ... ] event } ...
         *
         * Each event is prefixed with a timestamp,
         * which is encoded in a variable-length format.
         * The timestamp describes the amount of time that
         * must be elapsed before this event can be handled.
         *
         * After the timestamp, comes the event data.
         * The first byte of the event always has the high bit on,
         * and the remaining bytes always have the high bit off.
         *
         * The first byte can however be omitted; in that case,
         * it is assumed that the first byte is the same as in
         * the previous command. This is called "running status".
         * The event may furthermore beprefixed
         * with a number of meta events.
         */
       Flush();
       if(data != running_status) AddBytes(running_status = data);
       AddBytes(args...);
    }
    void AddEvent() { }

    template<typename... Args>
    void AddMetaEvent(uint8_t metatype, uint8_t nbytes, Args...args)
    {
        Flush();
        AddBytes(0xFF, metatype, nbytes, args...);
    }

    // Key-related parameters: channel number, note number, pressure
    void KeyOn(uint16_t ch, uint16_t n, uint16_t p)    { if(n>=0)AddEvent(0x90|ch, n, p); }
    void KeyOff(uint16_t ch, uint16_t n, uint16_t p)   { if(n>=0)AddEvent(0x80|ch, n, p); }
    void KeyTouch(uint16_t ch, uint16_t n, uint16_t p) { if(n>=0)AddEvent(0xA0|ch, n, p); }
    // Events with other types of parameters:
    void Control(uint16_t ch, uint16_t c, uint16_t v) { AddEvent(0xB0|ch, c, v); }
    void Patch(uint16_t ch, uint16_t patchno)    { AddEvent(0xC0|ch, patchno); }
    void Wheel(uint16_t ch, uint32_t value)
        { AddEvent(0xE0|ch, value&0x7F, (value>>7)&0x7F); }

    // Methods for appending metadata into the track:
    void AddText(uint16_t texttype, const std::string& text)
    {
        AddMetaEvent(texttype, text.size(), text);
    }
};

/* Define a class that encapsulates all methods needed to craft a MIDI file. */
class MIDIfile: public MIDIvec
{
protected:
    std::vector<MIDItrack> tracks;
    uint32_t deltaticks;
    std::ofstream file;
public:
    MIDIfile(const std::string& filename,uint32_t ppq)
        : MIDIvec(), tracks(), deltaticks(ppq), file(filename,std::ios::binary|std::ios::trunc)
    {
    }

    void AddLoopStart()  { (*this)[0].AddText(6, "loopStart"); }
    void AddLoopEnd()    { (*this)[0].AddText(6, "loopEnd"); }
    void SetTempo(uint32_t tempo)
    { (*this)[0].AddMetaEvent(0x51,3,  tempo>>16, tempo>>8, tempo); }

    MIDItrack& operator[] (uint32_t trackno)
    {
        if(trackno >= tracks.size())
        {
            tracks.resize(trackno+1);
        }

        return tracks[trackno];
    }

    void Finish()
    {
        clear();
        AddBytes(
            // MIDI signature (MThd and number 6)
            "MThd", 0,0,0,6,
            // Format number (1: multiple tracks, synchronous)
            0,1,
            tracks.size() >> 8, tracks.size(),
            deltaticks    >> 8, deltaticks);
        for(uint32_t a=0; a<tracks.size(); ++a)
        {
            // Add meta 0x2F to the track, indicating the track end:
            tracks[a].AddMetaEvent(0x2F, 0);
            // Add the track into the MIDI file:
            AddBytes("MTrk",
                tracks[a].size() >> 24,
                tracks[a].size() >> 16,
                tracks[a].size() >>  8,
                tracks[a].size() >>  0);
            insert(end(), tracks[a].begin(), tracks[a].end());
        }
        std::copy(begin(), end(), std::ostream_iterator<uint8_t>(file));
        file.flush();
    }
};

}

#endif
