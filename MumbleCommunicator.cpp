#include "MumbleCommunicator.hpp"

#include <cstring>
#include <functional>

namespace mumble {
    class MumlibCallback : public mumlib::BasicCallback {
    public:
        std::shared_ptr<mumlib::Mumlib> mum;
        MumbleCommunicator *communicator;

        // called by Mumlib when receiving audio from mumble server 
        virtual void audio(
                int target,
                int sessionId,
                int sequenceNumber,
                int16_t *pcm_data,
                uint32_t pcm_data_size) override {
            communicator->onIncomingPcmSamples(communicator->callId, sessionId, sequenceNumber, pcm_data, pcm_data_size);
        }

        virtual void channelState(
                std::string name,
                int32_t channel_id,
                int32_t parent,
                std::string description,
                std::vector<uint32_t> links,
                std::vector<uint32_t> inks_add,
                std::vector<uint32_t> links_remove,
                bool temporary,
                int32_t position) override {
            communicator->onIncomingChannelState(name, channel_id);
        }

        virtual void serverSync(
                std::string welcome_text,
                int32_t session,
                int32_t max_bandwidth,
                int64_t permissions) override {
            communicator->onServerSync();
        };

        /*
        virtual void onUserState(
                int32_t session,
                int32_t actor,
                std::string name,
                int32_t user_id,
                int32_t channel_id,
                int32_t mute,
                int32_t deaf,
                int32_t suppress,
                int32_t self_mute,
                int32_t self_deaf,
                std::string comment,
                int32_t priority_speaker,
                int32_t recording
                ) override {
            communicator->onUserState();
        };
        */

    };
}

mumble::MumbleCommunicator::MumbleCommunicator(boost::asio::io_service &ioService)
        : ioService(ioService),
          logger(log4cpp::Category::getInstance("MumbleCommunicator")) {
}

void mumble::MumbleCommunicator::connect(MumbleCommunicatorConfig &config) {

    callback.reset(new MumlibCallback());

    mumbleConf = config;

    mumConfig = mumlib::MumlibConfiguration();
    mumConfig.opusEncoderBitrate = config.opusEncoderBitrate;

    mum.reset(new mumlib::Mumlib(*callback, ioService, mumConfig));
    callback->communicator = this;
    callback->mum = mum;

    // IMPORTANT: comment these out when experimenting with onConnect
    if ( ! MUM_DELAYED_CONNECT ) {
        mum->connect(config.host, config.port, config.user, config.password);
        if ( mumbleConf.autodeaf ) {
            mum->sendUserState(mumlib::UserState::SELF_DEAF, true);
        }
    }
}

void mumble::MumbleCommunicator::onConnect() {
    if ( MUM_DELAYED_CONNECT ) {
        mum->connect(mumbleConf.host, mumbleConf.port, mumbleConf.user, mumbleConf.password);
    }

    if ( mumbleConf.comment.size() > 0 ) {
        mum->sendUserState(mumlib::UserState::COMMENT, mumbleConf.comment);
    }
    if ( mumbleConf.autodeaf ) {
        mum->sendUserState(mumlib::UserState::SELF_DEAF, true);
    }
}

void mumble::MumbleCommunicator::onDisconnect() {
    if ( MUM_DELAYED_CONNECT ) {
        mum->disconnect();
    } else {
    }
}

void mumble::MumbleCommunicator::onCallerAuth() {
    //onServerSync();
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

/*
void mumble::MumbleCommunicator::onUserState(
        int32_t session,
        int32_t actor,
        std::string name,
        int32_t user_id,
        int32_t channel_id,
        int32_t mute,
        int32_t deaf,
        int32_t suppress,
        int32_t self_mute,
        int32_t self_deaf,
        std::string comment,
        int32_t priority_speaker,
        int32_t recording) {

    logger::notice("Entered onUserState(...)");

    userState.mute = mute;
    userState.deaf = deaf;
    userState.suppress = suppress;
    userState.self_mute = self_mute;
    userState.self_deaf = self_deaf;
    userState.priority_speaker = priority_speaker;
    userState.recording = recording;
}
*/


void mumble::MumbleCommunicator::joinChannel(int channel_id) {
    mum->joinChannel(channel_id);

    if ( mumbleConf.autodeaf ) {
        mum->sendUserState(mumlib::UserState::SELF_DEAF, true);
    }
}


void mumble::MumbleCommunicator::sendUserState(mumlib::UserState field, bool val) {
    mum->sendUserState(field, val);
}

