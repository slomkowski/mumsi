#include "log4cpp/Category.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"

#include "PjsuaCommunicator.hpp"
#include "MumbleCommunicator.hpp"

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
    logger.setPriority(log4cpp::Priority::INFO);
    logger.addAppender(appender1);


    sip::PjsuaCommunicator pjsuaCommunicator(
            SIP_DOMAIN, SIP_USER, SIP_PASSWD);

    mumble::MumbleCommunicator mumbleCommunicator(
            pjsuaCommunicator,
            MUMBLE_USER, MUMBLE_PASSWD, MUMBLE_DOMAIN);

    logger.info("Application started.");

    mumbleCommunicator.loop();


    return 0;
}

