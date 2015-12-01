#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"
#include "IncomingConnectionValidator.hpp"
#include "MumbleChannelJoiner.hpp"
#include "Configuration.hpp"

#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>

int main(int argc, char *argv[]) {

    log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
    log4cpp::PatternLayout layout;
    layout.setConversionPattern("%d [%p] %c: %m%n");
    appender1->setLayout(&layout);
    log4cpp::Category &logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::NOTICE);
    logger.addAppender(appender1);

    if (argc == 1) {
        logger.crit("No configuration file provided. Use %s {config file}", argv[0]);
        return 1;
    }

    config::Configuration conf(argv[1]);

    sip::IncomingConnectionValidator connectionValidator(conf.getString("sip.validUriExpression"));

    boost::asio::io_service ioService;

    sip::PjsuaCommunicator pjsuaCommunicator(connectionValidator);

    mumble::MumbleCommunicator mumbleCommunicator(ioService);

    mumble::MumbleChannelJoiner mumbleChannelJoiner(conf.getString("mumble.channelNameExpression"));

    using namespace std::placeholders;
    pjsuaCommunicator.onIncomingPcmSamples = std::bind(
            &mumble::MumbleCommunicator::sendPcmSamples,
            &mumbleCommunicator,
            _1, _2);

    pjsuaCommunicator.onStateChange = std::bind(
            &mumble::MumbleCommunicator::sendTextMessage,
            &mumbleCommunicator, _1);

    mumbleCommunicator.onIncomingPcmSamples = std::bind(
            &sip::PjsuaCommunicator::sendPcmSamples,
            &pjsuaCommunicator,
            _1, _2, _3, _4);

    mumbleCommunicator.onIncomingChannelState = std::bind(
        &mumble::MumbleChannelJoiner::checkChannel,
        &mumbleChannelJoiner,
        _1, _2);

    mumbleCommunicator.onServerSync = std::bind(
        &mumble::MumbleChannelJoiner::maybeJoinChannel,
        &mumbleChannelJoiner,
        &mumbleCommunicator);


    mumbleCommunicator.connect(
            conf.getString("mumble.user"),
            conf.getString("mumble.password"),
            conf.getString("mumble.host"),
            conf.getInt("mumble.port"));

    pjsuaCommunicator.connect(
            conf.getString("sip.host"),
            conf.getString("sip.user"),
            conf.getString("sip.password"),
            conf.getInt("sip.port"));

    logger.info("Application started.");

    ioService.run();

    return 0;
}

