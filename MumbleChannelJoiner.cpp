#include "MumbleChannelJoiner.hpp"

#include <boost/algorithm/string.hpp>
using namespace std;

mumble::MumbleChannelJoiner::MumbleChannelJoiner(std::string channelNameRegex) : channelNameRegex(boost::regex(channelNameRegex)),
logger(log4cpp::Category::getInstance("MumbleChannelJoiner")){
    //std::vector<ChannelEntry> *channels = new std::vector<ChannelEntry>();
}

std::vector<mumble::MumbleChannelJoiner::ChannelEntry> mumble::MumbleChannelJoiner::channels;

void mumble::MumbleChannelJoiner::checkChannel(std::string channel_name, int channel_id) {
    boost::smatch s;
    ChannelEntry ent;
    logger.debug("Channel %s available (%d)", channel_name.c_str(), channel_id);

    ent.name = channel_name;
    ent.id = channel_id;

    channels.push_back(ent);

    if(boost::regex_match(channel_name, s, channelNameRegex)) {
      this->channel_id = channel_id;
    }
}

void mumble::MumbleChannelJoiner::maybeJoinChannel(mumble::MumbleCommunicator *mc) {
	if(channel_id > -1) {
		mc->joinChannel(channel_id);
	}
}

/* This is a secondary channel-switching object that relys on updates to the
 * class variable 'channels' for the channel list from the server.
 */
void mumble::MumbleChannelJoiner::findJoinChannel(mumble::MumbleCommunicator *mc) {
    boost::smatch s;

    int found = -1;

    for(std::vector<ChannelEntry>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if(boost::regex_match(it->name, s, channelNameRegex)) {
            found = it->id;
        }
    }

	if(found > -1) {
		mc->joinChannel(found);
	}
}

void mumble::MumbleChannelJoiner::joinOtherChannel(mumble::MumbleCommunicator *mc, std::string channelNameRegex) {
    this->channelNameRegex = boost::regex(channelNameRegex);
    findJoinChannel(mc);
}


