#pragma once

#include <pjmedia.h>

#include <log4cpp/Category.hh>
#include <boost/noncopyable.hpp>

#include <mutex>
#include <unordered_map>

namespace mixer {

    constexpr int MAX_BUFFER_LENGTH = 5000;

    class Exception : public std::runtime_error {
    public:
        Exception(std::string title) : std::runtime_error(title) {
            mesg += title;
        }

        Exception(std::string title, pj_status_t status) : std::runtime_error(title) {
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

    class AudioFramesMixer : boost::noncopyable {
    public:
        AudioFramesMixer(pj_pool_factory &poolFactory);

        virtual ~AudioFramesMixer();

        void addFrameToBuffer(int sessionId, int sequenceNumber, int16_t *samples, int samplesLength);

        int getMixedSamples(int16_t *mixedSamples, int requestedLength);

        void clear();

    private:
        log4cpp::Category &logger;

        pj_pool_t *pool = nullptr;

        std::unordered_map<int, pjmedia_circ_buf *> buffersMap;

        std::mutex inBuffAccessMutex;
    };

}
