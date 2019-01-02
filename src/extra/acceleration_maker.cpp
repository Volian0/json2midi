#include <cstdint>
#include <cmath>
#include <stdexcept>
#include "../midi.h"

int main(int argc, char* argv[])
{
    if (argc<5)
        throw std::logic_error("Not enough arguments");
    long double start_tpm{std::stold(argv[1])};
    long double acc{std::stold(argv[2])};
    uint64_t time{std::stoull(argv[3])*10};
    uint64_t current_tick{};
    midi::MIDIfile midi(1200);
    for (uint64_t i{}; i<time; ++i)
    {
        long double tpm = start_tpm + i*6*acc;
        long double tick_ld = i * (start_tpm + i*3*acc);
        if ((!std::isnormal(tick_ld)||tick_ld<0||tick_ld>uint64_t{}-1)&&!(tick_ld==0))
            throw std::logic_error("Error 1");
        uint64_t tick = tick_ld;
        if (current_tick>tick)
            throw std::logic_error("Error 2");
        midi[0].AddDelay(tick-current_tick);
        current_tick = tick;
        midi.SetTempo(120000000/tpm);
    }
    midi.Create(argv[4]);
    return 0;
}
