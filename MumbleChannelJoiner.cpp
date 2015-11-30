#include "MumbleChannelJoiner.hpp"

#include <boost/algorithm/string.hpp>
using namespace std;

mumble::MumbleChannelJoiner::MumbleChannelJoiner(std::string channelNameRegex) : channelNameRegex(boost::regex(channelNameRegex)),
logger(log4cpp::Category::getInstance("MumbleChannelJoiner")){
}

void mumble::MumbleChannelJoiner::checkChannel(std::string channel_name, int channel_id) {
    boost::smatch s;
    logger.debug("Channel %s available (%d)", channel_name.c_str(), channel_id);


    if(boost::regex_match(channel_name, s, channelNameRegex)) {
      this->channel_id = channel_id;
    }
}

void mumble::MumbleChannelJoiner::maybeJoinChannel(mumble::MumbleCommunicator *mc) {
	if(channel_id > -1) {
		mc->joinChannel(channel_id);
	}
}

