#ifndef MUMSI_MUMBLECOMMUNICATOR_HPP
#define MUMSI_MUMBLECOMMUNICATOR_HPP

#include "ISamplesBuffer.hpp"

extern "C" {
#include "libmumble.h"
}

#include <string>
#include <stdexcept>
#include <opus.h>
#include <log4cpp/Category.hh>
#include <sndfile.hh>
#include <thread>

namespace mumble {

    constexpr unsigned int SAMPLE_RATE = 48000;

    class Exception : public std::runtime_error {
    public:
        Exception(const char *message) : std::runtime_error(message) { }
    };

    class MumbleCommunicator {
    public:
        MumbleCommunicator(
                ISamplesBuffer &samplesBuffer,
                std::string user,
                std::string password,
                std::string host,
                int port = 0);

        ~MumbleCommunicator();

        void loop();

        void senderThreadFunction();

        void receiveAudioFrameCallback(uint8_t *audio_data, uint32_t audio_data_size);

    private:
        log4cpp::Category &logger;

        ISamplesBuffer &samplesBuffer;

        std::unique_ptr<std::thread> senderThread;

        mumble_struct *mumble;
        OpusDecoder *opusDecoder;
        OpusEncoder *opusEncoder;

        int outgoingAudioSequenceNumber;

        SndfileHandle fileHandle;

        bool quit;
    };
}

#endif //MUMSI_MUMBLECOMMUNICATOR_HPP
