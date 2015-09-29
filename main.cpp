#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include "log4cpp/Priority.hh"

#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"

#include "SoundSampleQueue.hpp"

#define SIP_DOMAIN "sip.antisip.com"
#define SIP_USER "melangtone"
#define SIP_PASSWD "b8DU9AZXbd9tVCWg"

#define MUMBLE_DOMAIN "1con.pl"
#define MUMBLE_USER "mumsi"
#define MUMBLE_PASSWD "kiwi"

int main(int argc, char *argv[]) {

    log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
    appender1->setLayout(new log4cpp::BasicLayout());
    log4cpp::Category &logger = log4cpp::Category::getRoot();
    logger.setPriority(log4cpp::Priority::DEBUG);
    logger.addAppender(appender1);

    SoundSampleQueue<int16_t> mumbleToSipQueue;
    SoundSampleQueue<int16_t> sipToMumbleQueue;

    mumble::MumbleCommunicator mumbleCommunicator(
            sipToMumbleQueue,
            mumbleToSipQueue,
            MUMBLE_USER, MUMBLE_PASSWD, MUMBLE_DOMAIN);

    pjsua::PjsuaCommunicator pjsuaCommunicator(
            mumbleToSipQueue,
            sipToMumbleQueue,
            SIP_DOMAIN, SIP_USER, SIP_PASSWD);

    logger.info("Application started.");

    pjsuaCommunicator.loop();

    mumbleCommunicator.loop();

    return 0;
}

