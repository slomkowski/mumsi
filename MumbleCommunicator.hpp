#pragma once

#include <mumlib.hpp>

#include <log4cpp/Category.hh>
#include <boost/noncopyable.hpp>

#include <string>
#include <stdexcept>

namespace mumble {

    class Exception : public std::runtime_error {
    public:
        Exception(const char *message) : std::runtime_error(message) { }
    };

    class MumlibCallback;

    class MumbleCommunicator : boost::noncopyable {
    public:
        MumbleCommunicator(
                boost::asio::io_service &ioService);

        void connect(
                std::string user,
                std::string password,
                std::string host,
                int port = 0);

        virtual ~MumbleCommunicator();

        void sendPcmSamples(int16_t *samples, unsigned int length);

        /**
         * This callback is called when communicator has received samples.
         * Arguments: session ID, sequence number, PCM samples, length of samples
         */
        std::function<void(int, int, int16_t *, int)> onIncomingPcmSamples;

        void sendTextMessage(std::string message);

    public:
        boost::asio::io_service &ioService;

        log4cpp::Category &logger;

        std::shared_ptr<mumlib::Mumlib> mum;

        std::unique_ptr<MumlibCallback> callback;

        friend class MumlibCallback;
    };
}
