#ifndef MUMSI_MUMBLECOMMUNICATOR_HPP
#define MUMSI_MUMBLECOMMUNICATOR_HPP

extern "C" {
#include "libmumble.h"
}

#include <string>
#include <stdexcept>
#include <opus.h>

namespace mumble {

    constexpr unsigned int SAMPLE_RATE = 48000;

    class Exception : public std::runtime_error {
    public:
        Exception(const char *message) : std::runtime_error(message) { }
    };

    class MumbleCommunicator {
    public:
        MumbleCommunicator(std::string user, std::string password, std::string host, int port = 0);

        ~MumbleCommunicator();

        void loop();

        void receiveAudioFrameCallback(uint8_t *audio_data, uint32_t audio_data_size);

    private:
        mumble_struct *mumble;
        OpusDecoder *opusState;
    };
}

#endif //MUMSI_MUMBLECOMMUNICATOR_HPP
