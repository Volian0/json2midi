#include "pt2.h"
#include "midi.h"
#include "experimental/json.h" //MEMORY LEAK!
#include <algorithm>
#include <cmath>

namespace pt2
{

uint8_t message::GetType() const
{
    return (value&0xF0000000)>>28;
}

message::message(uint32_t length,uint8_t type)
    : value((length&0x0FFFFFFF)|(type<<28)) {}

message::operator uint32_t() const
{
    return (value&0x0FFFFFFF);
}

void part::VerifyLength()
{
    if (tracks.empty())
        throw std::logic_error("No tracks");
    for (std::vector<track>::size_type i=1; i<tracks.size(); ++i)
    {
        int32_t diff = tracks[0]-tracks[i];
        if (diff<0)
        {
            tracks[i].warnings.push_back("Track is too long ("+std::to_string(diff)+" ticks) - TRACK WILL BE SHRINKED");
            tracks[i].shrink(-diff);
        }
        else if (diff>0)
        {
            tracks[i].warnings.push_back("Track is shorter than the first track ("+std::to_string(diff)+" ticks)");
            tracks[i].messages.push_back(message(diff,2));
        }
    }
}

bool part::HasWarnings() const
{
    for (const auto& t : tracks)
        if (!t.warnings.empty())
            return true;
    return false;
}

void track::parse(const std::string& score)
{
    uint8_t mode=0;
    std::vector<uint8_t> notes;
    for (std::string::size_type i=0; i<score.size(); ++i)
    {
        uint8_t b = score[i];
        if (b=='.')
        {
            if (mode==2)
                mode = 1;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
        }
        else if (b=='~')
        {
            if (mode==2)
                mode = 1;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
            notes.push_back(2);
        }
        else if (b=='@')
        {
            if (mode==2)
                mode = 1;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
            notes.push_back(3);
        }
        else if (b=='%')
        {
            if (mode==2)
                mode = 1;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
            notes.push_back(4);
        }
        else if (b=='(')
        {
            if (mode==0)
                mode = 1;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
        }
        else if (b==')')
        {
            if (mode==2)
                mode = 3;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
        }
        else if (b=='[')
        {
            if (mode==3)
                mode = 4;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
        }
        else if (b==']')
        {
            if (mode==6)
                mode = 5;
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
        }
        else if (b==','||b==';')
        {
            if (mode==5)
                mode = 0;
            else if (mode==0)
                warnings.push_back("Duplicated , or ;");
            else
                throw std::runtime_error(std::string("Unexpected ")+score[i]);
        }
        else
        {
            if (score[i]==' ')
            {
                warnings.push_back("Space in code");
                continue;
            }
            if ((score[i]=='<'||(score[i]>='0'&&score[i]<='9'))&&mode==0)
                continue;
            if ((score[i]=='>'||score[i]=='{'||score[i]=='}'||(score[i]>='0'&&score[i]<='9'))&&mode==5)
                continue;
            std::string temp;
            while (true)
            {
                temp+=score[i];
                ++i;
                if (i==score.size()||score[i]=='.'||score[i]=='('
                        ||score[i]==')'||score[i]=='~'||score[i]=='['
                        ||score[i]==']'||score[i]==','||score[i]==';'
                        ||score[i]=='<'||score[i]=='>'||score[i]=='@'
                        ||score[i]=='%')
                {
                    --i;
                    break;
                }
            }
            uint8_t note = GetNote(temp);
            uint32_t length = GetLength(temp,0);
            uint32_t rest = GetLength(temp,1);
            if (note)
            {
                if (mode==0)
                    mode = 3;
                else if (mode==1)
                    mode = 2;
                else
                    throw std::exception();
                if (note != 1)
                    notes.push_back(note);
            }
            else if (length)
            {
                if (mode==4)
                    mode = 6;
                else
                    throw std::exception();
                // // //flush notes
                uint32_t div = std::count(notes.begin(),notes.end(),2) + 1;
                uint32_t arp1 = std::count(notes.begin(),notes.end(),3); //@
                uint32_t arp2 = std::count(notes.begin(),notes.end(),4); //%
                if ((arp1>0&&div>1)||(arp2>0&&div>1)||(arp1>0&&arp2>0))
                    throw std::logic_error("Problem with arpeggios");
                safe_divider sdiv;
                std::vector<uint8_t> tempnotes;
                if (arp1)
                {
                    for (auto n = notes.begin(); n<=notes.end(); ++n)
                    {
                        if (n==notes.end())
                        {
                            messages.push_back(message(length,2));
                            for (auto tn : notes)
                                if (tn!=3)
                                    messages.push_back(message(tn,1));
                        }
                        else if (*n==3)
                        {
                            uint32_t delay;
                            if (arp1==1)
                                delay = sdiv.divide(length,10);
                            else
                                delay = sdiv.divide(length,10*(arp1-1));
                            if (delay>length)
                                throw std::logic_error("Fatal error with @");
                            length = length - delay;
                            messages.push_back(message(delay,2));
                        }
                        else
                            messages.push_back(message(*n,0));
                    }
                }
                else if (arp2)
                {
                    for (auto n = notes.begin(); n<=notes.end(); ++n)
                    {
                        if (n==notes.end())
                        {
                            messages.push_back(message(length,2));
                            for (auto tn : notes)
                                if (tn!=4)
                                    messages.push_back(message(tn,1));
                        }
                        else if (*n==4)
                        {
                            uint32_t delay;
                            delay = sdiv.divide(3*length,10*arp2);
                            if (delay>length)
                                throw std::logic_error("Fatal error with %");
                            length = length - delay;
                            messages.push_back(message(delay,2));
                        }
                        else
                            messages.push_back(message(*n,0));
                    }
                }
                else
                    for (auto n = notes.begin(); n<=notes.end(); ++n)
                    {
                        if (n==notes.end()||*n==2)
                        {
                            for (auto tn : tempnotes)
                                messages.push_back(message(tn,0));
                            messages.push_back(message(sdiv.divide(length,div),2));
                            for (auto tn : tempnotes)
                                messages.push_back(message(tn,1));
                            tempnotes.clear();
                        }
                        else
                            tempnotes.push_back(*n);
                    }
                notes.clear();
                // // //
            }
            else if (rest)
            {
                if (mode==0)
                {
                    mode = 5;
                    messages.push_back(message(rest,2));
                }
                else if (mode==1)
                {
                    warnings.push_back("\""+temp+"\" inside () converted to mute");
                    mode = 2;
                }
                else
                    throw std::exception();
            }
            else
                throw std::runtime_error("Couldn't parse \""+temp+"\"");
        }
    }
    if (mode!=0&&mode!=5)
        throw std::exception();
}

uint32_t track::GetLength(const std::string& n,bool mode) const
{
    uint32_t delay{};
    if (mode)
    {
        for (uint8_t b : n)
        {
            //Q-Y
            if (b=='Q')
                delay+= 256*basebeats;
            else if (b=='R')
                delay+= 128*basebeats;
            else if (b=='S')
                delay+= 64*basebeats;
            else if (b=='T')
                delay+= 32*basebeats;
            else if (b=='U')
                delay+= 16*basebeats;
            else if (b=='V')
                delay+= 8*basebeats;
            else if (b=='W')
                delay+= 4*basebeats;
            else if (b=='X')
                delay+= 2*basebeats;
            else if (b=='Y')
                delay+= 1*basebeats;
            else
                return 0;
            if(delay>0xFFFFFFF)
                throw std::overflow_error("Length overflow");
        }
    }
    else
    {
        for (uint8_t b : n)
        {
            //H-P
            if (b=='H')
                delay+= 256*basebeats;
            else if (b=='I')
                delay+= 128*basebeats;
            else if (b=='J')
                delay+= 64*basebeats;
            else if (b=='K')
                delay+= 32*basebeats;
            else if (b=='L')
                delay+= 16*basebeats;
            else if (b=='M')
                delay+= 8*basebeats;
            else if (b=='N')
                delay+= 4*basebeats;
            else if (b=='O')
                delay+= 2*basebeats;
            else if (b=='P')
                delay+= 1*basebeats;
            else
                return 0;
            if(delay>0xFFFFFFF)
                throw std::overflow_error("Length overflow");
        }
    }
    return delay;
}

int32_t track::operator-(const track& tr) const
{
    int32_t diff{};
    std::vector<message>::size_type a{};
    std::vector<message>::size_type b{};
    std::vector<message> msg_a{messages};
    std::vector<message> msg_b{tr.messages};
    while (a<msg_a.size()||b<msg_b.size())
    {
        while (a<msg_a.size())
        {
            if (msg_a[a]!=0&&msg_a[a].GetType()==2)
            {
                msg_a[a] = message(msg_a[a] - 1,2);
                ++diff;
                break;
            }
            else ++a;
        }
        while (b<msg_b.size())
        {
            if (msg_b[b]!=0&&msg_b[b].GetType()==2)
            {
                msg_b[b] = message(msg_b[b] - 1,2);
                --diff;
                break;
            }
            else ++b;
        }
        if (diff>0xFFFFFFF||diff<-0xFFFFFFF)
            throw std::overflow_error("Length overflow");
    }
    return diff;
}

void track::shrink(uint32_t a)
{
    for (std::vector<message>::size_type i{messages.size()}; i>0; --i)
    {
        if (messages[i-1].GetType()==2)
        {
            int32_t diff = a - messages[i-1];
            if (diff>=0)
            {
                a = diff;
                messages[i-1] = message(0,2);
            }
            else if (diff<0)
            {
                a = 0;
                messages[i-1] = message(-diff,2);
            }
            if (a==0)
                break;
        }
    }
    if (a!=0)
        throw std::logic_error("This shouldn't happen, contact Volian0");
    //note cleaning...
    std::vector<message*> note_on;
    for (auto& m : messages)
    {
        if (m.GetType()==2&&m>0)
            note_on.clear();
        else if (m.GetType()==0)
            note_on.push_back(&m);
        else if (m.GetType()==1)
            for (auto& n : note_on)
                if (n->GetType()==0&&m==*n)
                {
                    m=message(0,3);
                    *n=message(0,3);
                    break;
                }
    }
}

song::song(const std::vector<std::string>& args)
    : arguments{args}
{
    ParseJSON();
    VerifyTracks();
    MakeMIDI(args.at(0));
}

void song::ParseJSON()
{
    std::string name = arguments.at(0);
    arguments.erase(arguments.begin());
    bool automode = false;
    if (arguments.empty()||arguments.at(0)=="auto")
    {
        automode = true;
        arguments.clear();
    }
    const json::JsonFile file(name);
    json::JsonArray* musics = file.value->GetObject()->Find("musics")->GetArray();
    for (uint32_t p=0; p<musics->GetSize(); ++p)
    {
        try
        {
            std::string bpm;
            std::string basebeats;
            parts.push_back(part());
            //SET BPM AND BASEBEATS
            json::JsonObject* part = musics->GetValue(p)->GetObject();
            if (automode)
            {
                bpm = part->Find("bpm")->GetString(true);
                basebeats = part->Find("baseBeats")->GetString(true);
            }
            else
            {
                try
                {
                    bpm = arguments.at(0);
                    arguments.erase(arguments.begin());
                    basebeats = arguments.at(0);
                    arguments.erase(arguments.begin());
                }
                catch (const std::exception&)
                {
                    throw std::runtime_error("Error with arguments");
                }
            }
            // // // setting BPM and Basebeats
            if (basebeats=="15")
                parts.back().basebeats = 1;
            else if (basebeats=="7.5")
                parts.back().basebeats = 2;
            else if (basebeats=="5")
                parts.back().basebeats = 3;
            else if (basebeats=="3.75")
                parts.back().basebeats = 4;
            else if (basebeats=="3")
                parts.back().basebeats = 5;
            else if (basebeats=="2.5")
                parts.back().basebeats = 6;
            else if (basebeats=="1.875")
                parts.back().basebeats = 8;
            else if (basebeats=="1.5")
                parts.back().basebeats = 10;
            else if (basebeats=="1.25")
                parts.back().basebeats = 12;
            else if (basebeats=="1")
                parts.back().basebeats = 15;
            else if (basebeats=="0.9375")
                parts.back().basebeats = 16;
            else if (basebeats=="0.75")
                parts.back().basebeats = 20;
            else if (basebeats=="0.625")
                parts.back().basebeats = 24;
            else if (basebeats=="0.5")
                parts.back().basebeats = 30;
            else if (basebeats=="0.46875")
                parts.back().basebeats = 32;
            else if (basebeats=="0.375")
                parts.back().basebeats = 40;
            else if (basebeats=="0.3125")
                parts.back().basebeats = 48;
            else if (basebeats=="0.25")
                parts.back().basebeats = 60;
            else if (basebeats=="0.234375")
                parts.back().basebeats = 64;
            else if (basebeats=="0.1875")
                parts.back().basebeats = 80;
            else if (basebeats=="0.15625")
                parts.back().basebeats = 96;
            else if (basebeats=="0.125")
                parts.back().basebeats = 120;
            else if (basebeats=="0.1171875")
                parts.back().basebeats = 128;
            else if (basebeats=="0.09375")
                parts.back().basebeats = 160;
            else if (basebeats=="0.078125")
                parts.back().basebeats = 192;
            else if (basebeats=="0.0625")
                parts.back().basebeats = 240;
            else if (basebeats=="0.05859375")
                parts.back().basebeats = 256;
            else if (basebeats=="0.046875")
                parts.back().basebeats = 320;
            else if (basebeats=="0.0390625")
                parts.back().basebeats = 384;
            else if (basebeats=="0.03125")
                parts.back().basebeats = 480;
            else if (basebeats=="0.029296875")
                parts.back().basebeats = 512;
            else if (basebeats=="0.0234375")
                parts.back().basebeats = 640;
            else if (basebeats=="0.01953125")
                parts.back().basebeats = 768;
            else if (basebeats=="0.015625")
                parts.back().basebeats = 960;
            else
                throw std::invalid_argument("Wrong BaseBeats");
            parts.back().bpm = std::stold(bpm)*parts.back().basebeats;
            json::JsonArray* scores = part->Find("scores")->GetArray();
            for (uint32_t t=0; t<scores->GetSize(); ++t)
            {
                try
                {
                    //SET BASEBEATS
                    parts.back().tracks.push_back(track());
                    parts.back().tracks.back().basebeats = parts.back().basebeats;
                    std::string track = scores->GetValue(t)->GetString();
                    parts.back().tracks.back().parse(track);
                }
                catch(const std::exception& e)
                {
                    throw std::runtime_error("Track "+std::to_string(t+1)+":\n"+e.what());
                }
            }
            parts.back().VerifyLength();
        }
        catch(const std::exception& e)
        {
            throw std::runtime_error("Part "+std::to_string(p+1)+":\n"+e.what());
        }
    }
}

void song::VerifyTracks()
{
    uint32_t max_size{};
    for (const auto& p : parts)
    {
        if (p.tracks.size()>max_size)
            max_size = p.tracks.size();
    }
    for (auto& p : parts)
    {
        while (p.tracks.size()<max_size)
        {
            p.tracks.push_back(p.tracks.back());
            for (auto& m : p.tracks.back().messages)
            {
                if (m.GetType()<2)
                    m=message(0,3);
            }
        }
    }
}

void song::MakeMIDI(const std::string& name)
{
    std::string filename=name;
    if (name.size()>=5&&name.substr(name.size()-5,5)==".json")
        filename = name.substr(0,name.size()-5);
    filename += ".mid";
    midi::MIDIfile file(960);
    for (const auto& p : parts)
    {
        uint16_t track_id{};
        // // //
        if (p.bpm==0)
            throw std::invalid_argument("Dividing by 0");
        file.SetTempo(1800000000/p.bpm);
        // // //
        for (const auto& t : p.tracks)
        {
            // // //
            for (const auto& m : t.messages)
            {
                if (m.GetType()==0)
                    file[track_id].KeyOn(track_id%16,m,100);
                else if (m.GetType()==1)
                    file[track_id].KeyOff(track_id%16,m,64);
                else if (m.GetType()==2)
                    file[track_id].AddDelay(m);
            }
            // // //
            ++track_id;
        }
    }
    file.Create(filename);
}

uint32_t safe_divider::divide(uint32_t a,uint32_t b)
{
    if (b==0)
        throw std::invalid_argument("Dividing by 0");
    uint32_t c = a/b;
    remainder += a - (c*b);
    if (remainder>=b)
    {
        ++c;
        remainder = remainder-b;
    }
    return c;
}

uint8_t GetNote(const std::string& n)
{
    if (n=="c5")
        return 108;
    else if (n=="b4")
        return 107;
    else if (n=="#a4")
        return 106;
    else if (n=="a4")
        return 105;
    else if (n=="#g4")
        return 104;
    else if (n=="g4")
        return 103;
    else if (n=="#f4")
        return 102;
    else if (n=="f4")
        return 101;
    else if (n=="e4")
        return 100;
    else if (n=="#d4")
        return 99;
    else if (n=="d4")
        return 98;
    else if (n=="#c4")
        return 97;
    else if (n=="c4")
        return 96;
    else if (n=="b3")
        return 95;
    else if (n=="#a3")
        return 94;
    else if (n=="a3")
        return 93;
    else if (n=="#g3")
        return 92;
    else if (n=="g3")
        return 91;
    else if (n=="#f3")
        return 90;
    else if (n=="f3")
        return 89;
    else if (n=="e3")
        return 88;
    else if (n=="#d3")
        return 87;
    else if (n=="d3")
        return 86;
    else if (n=="#c3")
        return 85;
    else if (n=="c3")
        return 84;
    else if (n=="b2")
        return 83;
    else if (n=="#a2")
        return 82;
    else if (n=="a2")
        return 81;
    else if (n=="#g2")
        return 80;
    else if (n=="g2")
        return 79;
    else if (n=="#f2")
        return 78;
    else if (n=="f2")
        return 77;
    else if (n=="e2")
        return 76;
    else if (n=="#d2")
        return 75;
    else if (n=="d2")
        return 74;
    else if (n=="#c2")
        return 73;
    else if (n=="c2")
        return 72;
    else if (n=="b1")
        return 71;
    else if (n=="#a1")
        return 70;
    else if (n=="a1")
        return 69;
    else if (n=="#g1")
        return 68;
    else if (n=="g1")
        return 67;
    else if (n=="#f1")
        return 66;
    else if (n=="f1")
        return 65;
    else if (n=="e1")
        return 64;
    else if (n=="#d1")
        return 63;
    else if (n=="d1")
        return 62;
    else if (n=="#c1")
        return 61;
    else if (n=="c1")
        return 60;
    else if (n=="b")
        return 59;
    else if (n=="#a")
        return 58;
    else if (n=="a")
        return 57;
    else if (n=="#g")
        return 56;
    else if (n=="g")
        return 55;
    else if (n=="#f")
        return 54;
    else if (n=="f")
        return 53;
    else if (n=="e")
        return 52;
    else if (n=="#d")
        return 51;
    else if (n=="d")
        return 50;
    else if (n=="#c")
        return 49;
    else if (n=="c")
        return 48;
    else if (n=="B-1")
        return 47;
    else if (n=="#A-1")
        return 46;
    else if (n=="A-1")
        return 45;
    else if (n=="#G-1")
        return 44;
    else if (n=="G-1")
        return 43;
    else if (n=="#F-1")
        return 42;
    else if (n=="F-1")
        return 41;
    else if (n=="E-1")
        return 40;
    else if (n=="#D-1")
        return 39;
    else if (n=="D-1")
        return 38;
    else if (n=="#C-1")
        return 37;
    else if (n=="C-1")
        return 36;
    else if (n=="B-2")
        return 35;
    else if (n=="#A-2")
        return 34;
    else if (n=="A-2")
        return 33;
    else if (n=="#G-2")
        return 32;
    else if (n=="G-2")
        return 31;
    else if (n=="#F-2")
        return 30;
    else if (n=="F-2")
        return 29;
    else if (n=="E-2")
        return 28;
    else if (n=="#D-2")
        return 27;
    else if (n=="D-2")
        return 26;
    else if (n=="#C-2")
        return 25;
    else if (n=="C-2")
        return 24;
    else if (n=="B-3")
        return 23;
    else if (n=="#A-3")
        return 22;
    else if (n=="A-3")
        return 21;
    else if (n=="mute")
        return 1;
    else if (n=="empty")
        return 1;
    return 0;
}

}
