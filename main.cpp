#include <stdio.h>
#include "PjsuaCommunicator.hpp"
#include "PjsuaMediaPort.hpp"
#include "MumbleCommunicator.hpp"


#define SIP_DOMAIN "sip.antisip.com"
#define SIP_USER "melangtone"
#define SIP_PASSWD "b8DU9AZXbd9tVCWg"

#define MUMBLE_DOMAIN "1con.pl"
#define MUMBLE_USER "mumsi"
#define MUMBLE_PASSWD "kiwi"

int main(int argc, char *argv[]) {

    mumble::MumbleCommunicator mumbleCommunicator(MUMBLE_USER, MUMBLE_PASSWD, MUMBLE_DOMAIN);

    pjsua::PjsuaMediaPort mediaPort;
    pjsua::PjsuaCommunicator pjsuaCommunicator(SIP_DOMAIN, SIP_USER, SIP_PASSWD, mediaPort);


    pjsuaCommunicator.loop();

    mumbleCommunicator.loop();


    printf("Done\n");

    return 0;
}

