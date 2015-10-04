#ifndef MUMSI_PJSUAMEDIAPORT_HPP
#define MUMSI_PJSUAMEDIAPORT_HPP

//todo wywalić i wrzucić do nagłówka z definicjami
#include "AbstractCommunicator.hpp"

#include <pjmedia.h>
#include <sndfile.hh>

#include <log4cpp/Category.hh>

#include <string>
#include <stdexcept>

namespace pjsua {

    constexpr int SAMPLING_RATE = 48000;

    inline pj_str_t toPjString(std::string str) {
        return pj_str(const_cast<char *>(str.c_str()));
    }

    class Exception : public std::runtime_error {
    public:
        Exception(const char *title, pj_status_t status) : std::runtime_error(title) {
            //todo status code
        }
    };

    pj_status_t MediaPort_getFrame(pjmedia_port *port, pjmedia_frame *frame);

    pj_status_t MediaPort_putFrame(pjmedia_port *port, pjmedia_frame *frame);

    class PjsuaMediaPort {
    public:

        PjsuaMediaPort(
                SoundSampleQueue<SOUND_SAMPLE_TYPE> &inputQueue,
                SoundSampleQueue<SOUND_SAMPLE_TYPE> &outputQueue);

        ~PjsuaMediaPort();

        pjmedia_port *create_pjmedia_port();


    private:
        log4cpp::Category &logger;

        SoundSampleQueue<SOUND_SAMPLE_TYPE> &inputQueue;
        SoundSampleQueue<SOUND_SAMPLE_TYPE> &outputQueue;

        pjmedia_port *_pjmedia_port;

        SndfileHandle fileHandle;

        /**
        * For PJSUA implementation reasons, these callbacks have to be functions, not methods.
        * Since 'friend' usage.
        */
        friend pj_status_t MediaPort_getFrame(pjmedia_port *port,
                                              pjmedia_frame *frame);

        friend pj_status_t MediaPort_putFrame(pjmedia_port *port,
                                              pjmedia_frame *frame);
    };
}

#endif //MUMSI_PJSUAMEDIAPORT_HPP
