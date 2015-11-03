#pragma once

#include "ICommunicator.hpp"

#include <pjmedia.h>

#include <string>
#include <stdexcept>
#include <log4cpp/Category.hh>
#include <sndfile.hh>
#include <mutex>
#include <condition_variable>

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

    pj_status_t MediaPort_getFrameRawCallback(pjmedia_port *port, pjmedia_frame *frame);

    pj_status_t MediaPort_putFrameRawCallback(pjmedia_port *port, pjmedia_frame *frame);

    class PjsuaCommunicator : public ICommunicator {
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

    private:
        log4cpp::Category &logger;
        log4cpp::Category &callbackLogger;


        pjmedia_port *mediaPort;

        pjmedia_circ_buf *inputBuff;

        std::mutex inBuffAccessMutex;

        // todo make it completely stateless
        pjmedia_port *createMediaPort();

        void registerAccount(std::string host,
                             std::string user,
                             std::string password);

        pj_status_t mediaPortGetFrame(pjmedia_frame *frame);

        void mediaPortPutFrame(pj_int16_t *samples, pj_size_t count);

        /**
        * For PJMEDIA implementation reasons, these callbacks have to be functions, not methods.
        * That is the reason to use 'friend'.
        */
        friend pj_status_t MediaPort_getFrameRawCallback(pjmedia_port *port,
                                                         pjmedia_frame *frame);

        friend pj_status_t MediaPort_putFrameRawCallback(pjmedia_port *port,
                                                         pjmedia_frame *frame);
    };

}
