#ifndef MUMSI_PJSUAMEDIAPORT_HPP
#define MUMSI_PJSUAMEDIAPORT_HPP

#include <pjmedia.h>

#include <string>
#include <stdexcept>

namespace pjsua {

    constexpr int SAMPLING_RATE = 8000;

    inline pj_str_t toPjString(std::string str) {
        return pj_str(const_cast<char *>(str.c_str()));
    }

    class Exception : public std::runtime_error {
    public:
        Exception(const char *title, pj_status_t status) : std::runtime_error(title) {
            //todo status code
        }
    };

    class PjsuaMediaPort {
    public:

        PjsuaMediaPort() : _pjmedia_port(nullptr) { }

        ~PjsuaMediaPort();


        pjmedia_port *create_pjmedia_port();

    private:
        pjmedia_port *_pjmedia_port;
    };
}

#endif //MUMSI_PJSUAMEDIAPORT_HPP
