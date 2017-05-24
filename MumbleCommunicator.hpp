#pragma once

#include <mumlib.hpp>

#include <log4cpp/Category.hh>
#include <boost/noncopyable.hpp>

#include <string>
#include <stdexcept>

// 0 = mumble users connected at start; 1 = connect at dial-in
// TODO: fix mumlib::TransportException when this option is enabled
#define MUM_DELAYED_CONNECT 0

namespace mumble {


    class Exception : public std::runtime_error {
    public:
        Exception(const char *message) : std::runtime_error(message) { }
    };

    class MumlibCallback;

    struct MumbleCommunicatorConfig {
        std::string user;
        std::string password;
        std::string host;
        int opusEncoderBitrate;
        int port = 0;
        bool autodeaf;
        std::string comment;
        int max_calls = 1;
        std::string authchan; // config.ini: channelAuthExpression
    };

    // This is the subset that is of interest to us
    struct MumbleUserState {
        int32_t mute;
        int32_t deaf;
        int32_t suppress;
        int32_t self_mute;
        int32_t self_deaf;
        int32_t priority_speaker;
        int32_t recording;
    };

    class MumbleCommunicator : boost::noncopyable {
    public:
        MumbleCommunicator(
                boost::asio::io_service &ioService);

        void connect(MumbleCommunicatorConfig &config);
        void onConnect();
        void onDisconnect();
        void onCallerAuth();
        //void onCallerUnauth();

        virtual ~MumbleCommunicator();

        void sendPcmSamples(int16_t *samples, unsigned int length);

        /**
         * This callback is called when communicator has received samples.
         * Arguments: call ID, session ID, sequence number, PCM samples, length of samples
         */
        std::function<void(int, int, int, int16_t *, int)> onIncomingPcmSamples;

        /**
         * This callback is called when a channel state message (e.g. Channel
         * information) is received. Arguments: channel_id, name
         */
        std::function<void(std::string, int)> onIncomingChannelState;

        std::function<void()> onServerSync;

        std::function<void()> onUserState;

        void sendTextMessage(std::string message);

        void joinChannel(int channel_id);

        void sendUserState(mumlib::UserState field, bool val);

        MumbleUserState userState;

        int callId;

    private:
        boost::asio::io_service &ioService;

        log4cpp::Category &logger;

        MumbleCommunicatorConfig mumbleConf;

        mumlib::MumlibConfiguration mumConfig;

        std::shared_ptr<mumlib::Mumlib> mum;

        std::unique_ptr<MumlibCallback> callback;

        friend class MumlibCallback;

    };
}
