#ifndef MUMSI_MUMBLECOMMUNICATOR_HPP
#define MUMSI_MUMBLECOMMUNICATOR_HPP

#include "ISamplesBuffer.hpp"
#include <mumlib.hpp>

#include <string>
#include <stdexcept>
#include <log4cpp/Category.hh>
#include <sndfile.hh>
#include <thread>

namespace mumble {

    class Exception : public std::runtime_error {
    public:
        Exception(const char *message) : std::runtime_error(message) { }
    };

    class MumlibCallback;

    class MumbleCommunicator {
    public:
        MumbleCommunicator(
                boost::asio::io_service &ioService,
                ISamplesBuffer &samplesBuffer,
                std::string user,
                std::string password,
                std::string host,
                int port = 0);

        ~MumbleCommunicator();

//        void senderThreadFunction();

        //void receiveAudioFrameCallback(uint8_t *audio_data, uint32_t audio_data_size);


    public:
        boost::asio::io_service &ioService;

        log4cpp::Category &logger;

        ISamplesBuffer &samplesBuffer;

        std::shared_ptr<mumlib::Mumlib> mum;

        std::unique_ptr<std::thread> senderThread;

        SndfileHandle fileHandle;

        std::unique_ptr<MumlibCallback> callback;

        bool quit;

        friend class MumlibCallback;
    };
}

#endif //MUMSI_MUMBLECOMMUNICATOR_HPP
