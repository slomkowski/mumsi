#pragma once

#include "ICommunicator.hpp"

#include <pjmedia.h>
#include <pjsua-lib/pjsua.h>

#undef isblank

#include <log4cpp/Category.hh>
#include <boost/noncopyable.hpp>

#include <string>
#include <stdexcept>
#include <mutex>
#include <climits>

namespace sip {

    constexpr int DEFAULT_PORT = 5060;
    constexpr int SAMPLING_RATE = 48000;

    class Exception : public std::runtime_error {
    public:
        Exception(const char *title) : std::runtime_error(title) {
            mesg += title;
        }

        Exception(const char *title, pj_status_t status) : std::runtime_error(title) {
            char errorMsgBuffer[500];
            pj_strerror(status, errorMsgBuffer, sizeof(errorMsgBuffer));

            mesg += title;
            mesg += ": ";
            mesg += errorMsgBuffer;
        }

        virtual const char *what() const throw() {
            return mesg.c_str();
        }

    private:
        std::string mesg;
    };

    inline pj_str_t toPjString(std::string &str) {
        return pj_str(const_cast<char *>(str.c_str()));
    }

    class PjsuaCommunicator : public ICommunicator, boost::noncopyable {
    public:
        PjsuaCommunicator();

        void connect(
                std::string host,
                std::string user,
                std::string password,
                unsigned int port = DEFAULT_PORT);

        ~PjsuaCommunicator();

        virtual void sendPcmSamples(int16_t *samples, unsigned int length);

        std::function<void(std::string)> onStateChange;

        void onCallMediaState(pjsua_call_id call_id);

        void onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);

        void onDtmfDigit(pjsua_call_id callId, int digit);

        void onCallState(pjsua_call_id call_id, pjsip_event *e);

        pj_status_t mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame);

        pj_status_t mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame);

        pj_status_t _mediaPortOnDestroy(pjmedia_port *port);

    private:
        log4cpp::Category &logger;
        log4cpp::Category &callbackLogger;

        pjmedia_port mediaPort;
        int mediaPortSlot = INT_MIN;
        bool eof = false;

        pj_pool_t *pool = nullptr;

        pjmedia_circ_buf *inputBuff;

        std::mutex inBuffAccessMutex;

        bool available = true;

        void createMediaPort();

        void registerAccount(std::string host,
                             std::string user,
                             std::string password);

    };

}
