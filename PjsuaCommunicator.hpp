#pragma once

#include "IncomingConnectionValidator.hpp"
#include "AudioFramesMixer.hpp"

#include <pjmedia.h>
#include <pjsua-lib/pjsua.h>

#include <pjsua2.hpp>

#undef isblank

#include <log4cpp/Category.hh>
#include <boost/noncopyable.hpp>

#include <string>
#include <stdexcept>
#include <climits>
#include <bits/unique_ptr.h>

// for userState enum
#include <mumlib.hpp>

#include "main.hpp"

enum dtmf_modes_t {DTMF_MODE_UNAUTH, DTMF_MODE_ROOT, DTMF_MODE_STAR};

namespace sip {

    constexpr int DEFAULT_PORT = 5060;
    constexpr int SAMPLING_RATE = 48000;
    constexpr int MAX_CALLER_PIN_LEN = 64;
    constexpr int MAX_PIN_FAILS = 2;

    class Exception : public std::runtime_error {
    public:
        Exception(const char *title) : std::runtime_error(title) {
            mesg += title;
        }

        Exception(std::string title) : std::runtime_error(title) {
            mesg += title;
        }

        Exception(const char *title, pj_status_t status) : std::runtime_error(title) {
            char errorMsgBuffer[500];
            pj_strerror(status, errorMsgBuffer, sizeof(errorMsgBuffer));

            mesg += title;
            mesg += ": ";
            mesg += errorMsgBuffer;
        }

        virtual const char *what() const throw() override {
            return mesg.c_str();
        }

    private:
        std::string mesg;
    };

    class _LogWriter;

    class _Account;

    class _Call;

    class _MumlibAudioMedia;

    struct call {
        unsigned index;
        std::unique_ptr<mixer::AudioFramesMixer> mixer;
        std::unique_ptr<sip::_MumlibAudioMedia> media;
        pj_caching_pool cachingPool;
        std::function<void(std::string)> onStateChange;
        std::function<void(int16_t *, int)> onIncomingPcmSamples;
        std::function<void(int)> onMuteDeafChange;
        std::function<void(mumlib::UserState field, bool val)> sendUserState;
        std::function<void(mumlib::UserState field, std::string val)> sendUserStateStr;
        std::function<void()> onConnect;
        std::function<void()> onDisconnect;
        std::function<void()> onCallerAuth;
        std::function<void()> joinAuthChannel;
        std::function<void()> joinDefaultChannel;
    };

    class PjsuaCommunicator : boost::noncopyable {
    public:
        PjsuaCommunicator(IncomingConnectionValidator &validator, int frameTimeLength, int maxCalls);

        void connect(
                std::string host,
                std::string user,
                std::string password,
                unsigned int port = DEFAULT_PORT);

        virtual ~PjsuaCommunicator();

        void sendPcmSamples(
                int callId,
                int sessionId,
                int sequenceNumber,
                int16_t *samples,
                unsigned int length);

        // config params we get from config.ini
        std::string caller_pin;
        std::string file_welcome;
        std::string file_prompt_pin;
        std::string file_entering_channel;
        std::string file_announce_new_caller;
        std::string file_invalid_pin;
        std::string file_goodbye;
        std::string file_mute_on;
        std::string file_mute_off;
        std::string file_menu;

        // TODO: move these to private?
        std::string got_dtmf;
        dtmf_modes_t dtmf_mode = DTMF_MODE_ROOT;
        int pin_fails = 0;

        pj_status_t mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame);

        pj_status_t mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame);

        call calls[MY_MAX_CALLS];

    private:
        log4cpp::Category &logger;
        log4cpp::Category &pjsuaLogger;

        std::unique_ptr<_LogWriter> logWriter;
        std::unique_ptr<_Account> account;

        pj::Endpoint endpoint;

        IncomingConnectionValidator &uriValidator;

        void registerAccount(std::string host,
                             std::string user,
                             std::string password);

        friend class _Call;

        friend class _Account;

        friend class _MumlibAudioMedia;
    };

}
