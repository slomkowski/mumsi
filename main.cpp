#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"
#include "IncomingConnectionValidator.hpp"
#include "MumbleChannelJoiner.hpp"
#include "Configuration.hpp"

#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>

#include <execinfo.h>

/*
 * Code from http://stackoverflow.com/a/77336/5419223
 */
static void sigsegv_handler(int sig) {
    constexpr int STACK_DEPTH = 10;
    void *array[STACK_DEPTH];

    size_t size = backtrace(array, STACK_DEPTH);

    fprintf(stderr, "ERROR: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(int argc, char *argv[]) {
    signal(SIGSEGV, sigsegv_handler);

    log4cpp::OstreamAppender appender("console", &std::cout);
    log4cpp::PatternLayout layout;
    layout.setConversionPattern("%d [%p] %c: %m%n");
    appender.setLayout(&layout);
    log4cpp::Category &logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::NOTICE);
    logger.addAppender(appender);

    if (argc == 1) {
        logger.crit("No configuration file provided. Use %s {config file}", argv[0]);
        std::exit(1);
    }

    config::Configuration conf(argv[1]);

    logger.setPriority(log4cpp::Priority::getPriorityValue(conf.getString("general.logLevel")));

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

    mumble::MumbleCommunicatorConfig mumbleConf;
    mumbleConf.host = conf.getString("mumble.host");
    mumbleConf.port = conf.getInt("mumble.port");
    mumbleConf.user = conf.getString("mumble.user");
    mumbleConf.password = conf.getString("mumble.password");
    mumbleConf.opusEncoderBitrate = conf.getInt("mumble.opusEncoderBitrate");

    mumbleCommunicator.connect(mumbleConf);

    pjsuaCommunicator.connect(
            conf.getString("sip.host"),
            conf.getString("sip.user"),
            conf.getString("sip.password"),
            conf.getInt("sip.port"));

    logger.info("Application started.");

    ioService.run();

    return 0;
}

