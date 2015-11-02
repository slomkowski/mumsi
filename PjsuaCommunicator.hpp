#ifndef MUMSI_PJSUACOMMUNICATOR_HPP
#define MUMSI_PJSUACOMMUNICATOR_HPP

#include "ISamplesBuffer.hpp"

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

    inline pj_str_t toPjString(std::string str) {
        return pj_str(const_cast<char *>(str.c_str()));
    }

    pj_status_t MediaPort_getFrameRawCallback(pjmedia_port *port, pjmedia_frame *frame);

    pj_status_t MediaPort_putFrameRawCallback(pjmedia_port *port, pjmedia_frame *frame);

    class PjsuaCommunicator : public ISamplesBuffer {
    public:
        PjsuaCommunicator();

        void connect(
                std::string host,
                std::string user,
                std::string password,
                unsigned int port = DEFAULT_PORT);

        ~PjsuaCommunicator();

        virtual void pushSamples(int16_t *samples, unsigned int length);

        std::function<void(int16_t *, int)> onIncomingSamples;

//        virtual unsigned int pullSamples(int16_t *samples, unsigned int length, bool waitWhenEmpty);

    private:
        log4cpp::Category &logger;
        log4cpp::Category &callbackLogger;


        pjmedia_port *mediaPort;

        pjmedia_circ_buf *inputBuff;
        pjmedia_circ_buf *outputBuff;

        std::mutex inBuffAccessMutex;

        std::mutex outBuffAccessMutex;
        std::condition_variable outBuffCondVar;


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

#endif //MUMSI_PJSUACOMMUNICATOR_HPP
