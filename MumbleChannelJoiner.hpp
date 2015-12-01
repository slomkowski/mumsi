#pragma once

#include <boost/noncopyable.hpp>
#include <log4cpp/Category.hh>

#include <string>
#include <boost/regex.hpp>
#include "MumbleCommunicator.hpp"

namespace mumble {
    class MumbleChannelJoiner : boost::noncopyable {
    public:
        MumbleChannelJoiner(std::string channelNameRegex);

        void checkChannel(std::string channel_name, int channel_id);
        void maybeJoinChannel(mumble::MumbleCommunicator *mc);

    private:
        log4cpp::Category &logger;
        boost::regex channelNameRegex;
        int channel_id;
    };
}
