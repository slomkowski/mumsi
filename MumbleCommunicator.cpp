#include <cstring>
#include <functional>

#include "MumbleCommunicator.hpp"

namespace mumble {
    class MumlibCallback : public mumlib::BasicCallback {
    public:
        std::shared_ptr<mumlib::Mumlib> mum;
        MumbleCommunicator *communicator;

        virtual void audio(
                int16_t *pcm_data,
                uint32_t pcm_data_size) {
            communicator->samplesBuffer.pushSamples(pcm_data, pcm_data_size);
        }
    };
}

mumble::MumbleCommunicator::MumbleCommunicator(
        boost::asio::io_service &ioService,
        ISamplesBuffer &samplesBuffer,
        std::string user,
        std::string password,
        std::string host,
        int port) : ioService(ioService),
                    samplesBuffer(samplesBuffer),
                    logger(log4cpp::Category::getInstance("MumbleCommunicator")) {

    quit = false;

    callback.reset(new MumlibCallback());

    mum.reset(new mumlib::Mumlib(*callback, ioService));
    callback->communicator = this;
    callback->mum = mum;

    mum->connect(host, port, user, password);
}

mumble::MumbleCommunicator::~MumbleCommunicator() {
    mum->disconnect();
}
//
//void mumble::MumbleCommunicator::senderThreadFunction() {
//    while (!quit) {
//
//    }
//}
