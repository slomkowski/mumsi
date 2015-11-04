#include "MumbleCommunicator.hpp"

#include <cstring>
#include <functional>

namespace mumble {
    class MumlibCallback : public mumlib::BasicCallback {
    public:
        std::shared_ptr<mumlib::Mumlib> mum;
        MumbleCommunicator *communicator;

        virtual void audio(
                int16_t *pcm_data,
                uint32_t pcm_data_size) {
            communicator->onIncomingPcmSamples(pcm_data, pcm_data_size);
        }
    };
}

mumble::MumbleCommunicator::MumbleCommunicator(boost::asio::io_service &ioService)
        : ioService(ioService),
          logger(log4cpp::Category::getInstance("MumbleCommunicator")) {
}

void mumble::MumbleCommunicator::connect(
        std::string user,
        std::string password,
        std::string host,
        int port) {

    callback.reset(new MumlibCallback());

    mum.reset(new mumlib::Mumlib(*callback, ioService));
    callback->communicator = this;
    callback->mum = mum;

    mum->connect(host, port, user, password);
}

void mumble::MumbleCommunicator::sendPcmSamples(int16_t *samples, unsigned int length) {
    mum->sendAudioData(samples, length);
}

mumble::MumbleCommunicator::~MumbleCommunicator() {
    mum->disconnect();
}

void mumble::MumbleCommunicator::sendTextMessage(std::string message) {
    mum->sendTextMessage(message);
}
