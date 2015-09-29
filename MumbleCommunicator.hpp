#ifndef MUMSI_MUMBLECOMMUNICATOR_HPP
#define MUMSI_MUMBLECOMMUNICATOR_HPP

#include "AbstractCommunicator.hpp"

extern "C" {
#include "libmumble.h"
}

#include <string>
#include <stdexcept>
#include <opus.h>
#include <log4cpp/Category.hh>

namespace mumble {

    constexpr unsigned int SAMPLE_RATE = 48000;

    class Exception : public std::runtime_error {
    public:
        Exception(const char *message) : std::runtime_error(message) { }
    };

    class MumbleCommunicator : public AbstractCommunicator {
    public:
        MumbleCommunicator(
                SoundSampleQueue<SOUND_SAMPLE_TYPE> &inputQueue,
                SoundSampleQueue<SOUND_SAMPLE_TYPE> &outputQueue,
                std::string user,
                std::string password,
                std::string host,
                int port = 0);

        ~MumbleCommunicator();

        void loop();

        void receiveAudioFrameCallback(uint8_t *audio_data, uint32_t audio_data_size);

    private:
        log4cpp::Category &logger;

        mumble_struct *mumble;
        OpusDecoder *opusDecoder;
        OpusEncoder *opusEncoder;

        int outgoingAudioSequenceNumber;
    };
}

#endif //MUMSI_MUMBLECOMMUNICATOR_HPP
