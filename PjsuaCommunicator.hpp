#ifndef MUMSI_PJSUACOMMUNICATOR_HPP
#define MUMSI_PJSUACOMMUNICATOR_HPP

#include "PjsuaMediaPort.hpp"
#include "AbstractCommunicator.hpp"

#include <pjmedia.h>

#include <string>
#include <stdexcept>

namespace pjsua {

    constexpr int SIP_PORT = 5060;

    class PjsuaCommunicator : public AbstractCommunicator {
    public:
        PjsuaCommunicator(
                SoundSampleQueue<SOUND_SAMPLE_TYPE> &inputQueue,
                SoundSampleQueue<SOUND_SAMPLE_TYPE> &outputQueue,
                std::string host,
                std::string user,
                std::string password);

        ~PjsuaCommunicator();

        void loop();

    private:
        PjsuaMediaPort mediaPort;
    };

}

#endif //MUMSI_PJSUACOMMUNICATOR_HPP
