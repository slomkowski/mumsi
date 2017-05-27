#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"
#include "IncomingConnectionValidator.hpp"
#include "MumbleChannelJoiner.hpp"
#include "Configuration.hpp"

#include <log4cpp/FileAppender.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/PatternLayout.hh>

#include <execinfo.h>

#include "main.hpp"

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
    int max_calls;

    log4cpp::OstreamAppender appender("console", &std::cout);
    log4cpp::PatternLayout layout;
    layout.setConversionPattern("%d [%p] %c: %m%n");
    appender.setLayout(&layout);
    log4cpp::Category &logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::DEBUG);
    //logger.setPriority(log4cpp::Priority::NOTICE);
    logger.addAppender(appender);

    if (argc == 1) {
        logger.crit("No configuration file provided. Use %s {config file}", argv[0]);
        std::exit(1);
    }

    config::Configuration conf(argv[1]);

    logger.setPriority(log4cpp::Priority::getPriorityValue(conf.getString("general.logLevel")));

    sip::IncomingConnectionValidator connectionValidator(conf.getString("sip.validUriExpression"));

    boost::asio::io_service ioService;

    try {
        max_calls = conf.getInt("sip.max_calls");
    } catch (...) {
        max_calls = 1;
    }

    sip::PjsuaCommunicator pjsuaCommunicator(connectionValidator, conf.getInt("sip.frameLength"), max_calls);

    mumble::MumbleCommunicatorConfig mumbleConf;
    mumbleConf.host = conf.getString("mumble.host");
    mumbleConf.port = conf.getInt("mumble.port");
    mumbleConf.user = conf.getString("mumble.user");
    mumbleConf.password = conf.getString("mumble.password");
    mumbleConf.opusEncoderBitrate = conf.getInt("mumble.opusEncoderBitrate");
    /* default to 'false' if not found */
    try {
        mumbleConf.autodeaf = conf.getBool("mumble.autodeaf");
    } catch (...) {
        mumbleConf.autodeaf = false;
    }

    try {
        mumbleConf.comment = conf.getString("app.comment");
    } catch (...) {
        mumbleConf.comment = "";
    }

    try {
        mumbleConf.authchan = conf.getString("mumble.channelAuthExpression");
    } catch (...) {
        mumbleConf.authchan = "";
    }

    /* default to <no pin> */
    try {
        pjsuaCommunicator.caller_pin = conf.getString("app.caller_pin");
    } catch (...) {
        pjsuaCommunicator.caller_pin = "";
    }

    try { pjsuaCommunicator.file_welcome = conf.getString("files.welcome");
    } catch (...) {
        pjsuaCommunicator.file_welcome = "welcome.wav";
    }

    try { pjsuaCommunicator.file_prompt_pin = conf.getString("files.prompt_pin");
    } catch (...) {
        pjsuaCommunicator.file_prompt_pin = "prompt-pin.wav";
    }

    try { pjsuaCommunicator.file_entering_channel = conf.getString("files.entering_channel");
    } catch (...) {
        pjsuaCommunicator.file_entering_channel = "entering-channel.wav";
    }

    try { pjsuaCommunicator.file_announce_new_caller = conf.getString("files.announce_new_caller");
    } catch (...) {
        pjsuaCommunicator.file_announce_new_caller = "announce-new-caller.wav";
    }

    try { pjsuaCommunicator.file_invalid_pin = conf.getString("files.invalid_pin");
    } catch (...) {
        pjsuaCommunicator.file_invalid_pin = "invalid-pin.wav";
    }

    try { pjsuaCommunicator.file_goodbye = conf.getString("files.goodbye");
    } catch (...) {
        pjsuaCommunicator.file_goodbye = "goodbye.wav";
    }

    try { pjsuaCommunicator.file_mute_on = conf.getString("files.mute_on");
    } catch (...) {
        pjsuaCommunicator.file_mute_on = "mute-on.wav";
    }

    try { pjsuaCommunicator.file_mute_off = conf.getString("files.mute_off");
    } catch (...) {
        pjsuaCommunicator.file_mute_off = "mute-off.wav";
    }

    try { pjsuaCommunicator.file_menu = conf.getString("files.menu");
    } catch (...) {
        pjsuaCommunicator.file_menu = "menu.wav";
    }

    /* If the channelUnauthExpression is set, use this as the default
     * channel and use channelNameExpression for the authchan. Otherwise,
     * use the channelNameExpression for the default.
     */
    std::string defaultChan = conf.getString("mumble.channelNameExpression"); 
    std::string authChan = "";

    if ( pjsuaCommunicator.caller_pin.size() > 0 ) {
        try {
            authChan = conf.getString("mumble.channelAuthExpression");
        } catch (...) {
        //    defaultChan = conf.getString("mumble.channelNameExpression"); 
        }
    }

    mumble::MumbleChannelJoiner mumbleChannelJoiner(defaultChan);
    mumble::MumbleChannelJoiner mumbleAuthChannelJoiner(authChan);

    for (int i = 0; i<max_calls; i++) {

        auto *mumcom = new mumble::MumbleCommunicator(ioService);
        mumcom->callId = i;

        using namespace std::placeholders;
        // Passing audio input from SIP to Mumble
        pjsuaCommunicator.calls[i].onIncomingPcmSamples = std::bind(
                &mumble::MumbleCommunicator::sendPcmSamples,
                mumcom,
                _1, _2);

        // PJ sends text message to Mumble
        pjsuaCommunicator.calls[i].onStateChange = std::bind(
                &mumble::MumbleCommunicator::sendTextMessage,
                mumcom,
                _1);

        /*
        // Send mute/deaf to Mumble
        pjsuaCommunicator.calls[i].onMuteDeafChange = std::bind(
                &mumble::MumbleCommunicator::mutedeaf,
                mumcom,
                _1);
         */

        // Send UserState to Mumble
        pjsuaCommunicator.calls[i].sendUserState = std::bind(
                static_cast<void(mumble::MumbleCommunicator::*)(mumlib::UserState, bool)>
                (&mumble::MumbleCommunicator::sendUserState),
                mumcom,
                _1, _2);

        // Send UserState to Mumble
        pjsuaCommunicator.calls[i].sendUserStateStr = std::bind(
                static_cast<void(mumble::MumbleCommunicator::*)(mumlib::UserState, std::string)>
                (&mumble::MumbleCommunicator::sendUserState),
                mumcom,
                _1, _2);

        // PJ triggers Mumble connect
        pjsuaCommunicator.calls[i].onConnect = std::bind(
                &mumble::MumbleCommunicator::onConnect,
                mumcom);

        // PJ triggers Mumble disconnect
        pjsuaCommunicator.calls[i].onDisconnect = std::bind(
                &mumble::MumbleCommunicator::onDisconnect,
                mumcom);

        // PJ notifies Mumble that Caller Auth is done
        pjsuaCommunicator.calls[i].onCallerAuth = std::bind(
                &mumble::MumbleCommunicator::onCallerAuth,
                mumcom);

        /*
        // PJ notifies Mumble that Caller Auth is done
        pjsuaCommunicator.calls[i].onCallerUnauth = std::bind(
                &mumble::MumbleCommunicator::onCallerUnauth,
                mumcom);
                */

        // PJ notifies Mumble that Caller Auth is done
        pjsuaCommunicator.calls[i].joinDefaultChannel = std::bind(
                &mumble::MumbleChannelJoiner::findJoinChannel,
                &mumbleChannelJoiner,
                mumcom);

        // PJ notifies Mumble that Caller Auth is done
        pjsuaCommunicator.calls[i].joinAuthChannel = std::bind(
                &mumble::MumbleChannelJoiner::findJoinChannel,
                &mumbleAuthChannelJoiner,
                mumcom);

        // Passing audio from Mumble to SIP
        mumcom->onIncomingPcmSamples = std::bind(
                &sip::PjsuaCommunicator::sendPcmSamples,
                &pjsuaCommunicator,
                _1, _2, _3, _4, _5);

        // Handle Channel State messages from Mumble
        mumcom->onIncomingChannelState = std::bind(
                &mumble::MumbleChannelJoiner::checkChannel,
                &mumbleChannelJoiner,
                _1, _2);

        // Handle Server Sync message from Mumble
        mumcom->onServerSync = std::bind(
                &mumble::MumbleChannelJoiner::maybeJoinChannel,
                &mumbleChannelJoiner,
                mumcom);

        mumbleConf.user = conf.getString("mumble.user") + '-' + std::to_string(i);
        mumcom->connect(mumbleConf);
    }

    pjsuaCommunicator.connect(
            conf.getString("sip.host"),
            conf.getString("sip.user"),
            conf.getString("sip.password"),
            conf.getInt("sip.port"));

    logger.info("Application started.");

    ioService.run();

    return 0;
}

