#ifndef MUMSI_PJSUACOMMUNICATOR_HPP
#define MUMSI_PJSUACOMMUNICATOR_HPP

#include "PjsuaMediaPort.hpp"

#include <pjmedia.h>

#include <string>
#include <stdexcept>

namespace pjsua {

    constexpr int SIP_PORT = 5060;

    class PjsuaCommunicator {
    public:
        PjsuaCommunicator(std::string host,
                          std::string user,
                          std::string password,
                          PjsuaMediaPort &mediaPort);

        ~PjsuaCommunicator();

        void loop();

    private:
        PjsuaMediaPort &mediaPort;
    };

}

#endif //MUMSI_PJSUACOMMUNICATOR_HPP
