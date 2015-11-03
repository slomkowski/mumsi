#include "log4cpp/Category.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"

#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"

#include "Configuration.hpp"

int main(int argc, char *argv[]) {

    log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
    appender1->setLayout(new log4cpp::BasicLayout());
    log4cpp::Category &logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::DEBUG);
    logger.addAppender(appender1);

    if (argc == 1) {
        logger.crit("No configuration file provided. Use %s {config file}", argv[0]);
        return 1;
    }

    config::Configuration conf(argv[1]);

    boost::asio::io_service ioService;

    sip::PjsuaCommunicator pjsuaCommunicator;

    mumble::MumbleCommunicator mumbleCommunicator(ioService);

    using namespace std::placeholders;
    pjsuaCommunicator.onIncomingPcmSamples = std::bind(
            &mumble::MumbleCommunicator::sendPcmSamples,
            &mumbleCommunicator,
            _1, _2);

    mumbleCommunicator.onIncomingPcmSamples = std::bind(
            &sip::PjsuaCommunicator::sendPcmSamples,
            &pjsuaCommunicator,
            _1, _2);

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

