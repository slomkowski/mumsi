#include "PjsuaCommunicator.hpp"


#include <pjlib.h>
#include <pjsua-lib/pjsua.h>

#include <functional>

//todo wywalić
#define THIS_FILE "mumsi"

using namespace std;


/**
 * This is global, because there's no way to pass it's value to onCallMediaState callback.
 */
static int mediaPortSlot;


static void onCallMediaState(pjsua_call_id call_id) {
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        pjsua_conf_connect(ci.conf_slot, mediaPortSlot);
        pjsua_conf_connect(mediaPortSlot, ci.conf_slot);
    }
}

static void onIncomingCall(pjsua_acc_id acc_id,
                           pjsua_call_id call_id,
                           pjsip_rx_data *rdata) {
    pjsua_call_info ci;

    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);

    pjsua_call_get_info(call_id, &ci);

    PJ_LOG(3, (THIS_FILE, "Incoming call from %.*s!!",
            (int) ci.remote_info.slen,
            ci.remote_info.ptr));

    /* Automatically answer incoming calls with 200/OK */
    pjsua_call_answer(call_id, 200, NULL, NULL);
}

static void onCallState(pjsua_call_id call_id,
                        pjsip_event *e) {
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &ci);
    PJ_LOG(3, (THIS_FILE, "Call %d state=%.*s", call_id,
            (int) ci.state_text.slen,
            ci.state_text.ptr));
}


pjsua::PjsuaCommunicator::PjsuaCommunicator(std::string host,
                                            std::string user,
                                            std::string password,
                                            PjsuaMediaPort &mediaPort) : mediaPort(mediaPort) {

    pj_status_t status;

    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        throw pjsua::Exception("Error in pjsua_create()", status);
    }

    /* Init pjsua */
    pjsua_config generalConfig;
    pjsua_config_default(&generalConfig);

    using namespace std::placeholders;

    generalConfig.cb.on_incoming_call = &onIncomingCall;
    generalConfig.cb.on_call_media_state = &onCallMediaState;
    generalConfig.cb.on_call_state = &onCallState;

    //todo zrobić coś z logami
    pjsua_logging_config logConfig;
    pjsua_logging_config_default(&logConfig);

    logConfig.console_level = 4;

    status = pjsua_init(&generalConfig, &logConfig, NULL);
    if (status != PJ_SUCCESS) {
        throw pjsua::Exception("Error in pjsua_init()", status);
    }

    /* Add UDP transport. */
    pjsua_transport_config transportConfig;
    pjsua_transport_config_default(&transportConfig);

    transportConfig.port = pjsua::SIP_PORT;

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transportConfig, NULL);
    if (status != PJ_SUCCESS) {
        throw pjsua::Exception("Error creating transport", status);
    }

    pjsua_set_null_snd_dev();

    pj_caching_pool cachingPool;
    pj_caching_pool_init(&cachingPool, &pj_pool_factory_default_policy, 0);
    pj_pool_t *pool = pj_pool_create(&cachingPool.factory, "wav", 4096, 4096, nullptr);

    pjsua_conf_add_port(pool, mediaPort.create_pjmedia_port(), &mediaPortSlot);

    /* Initialization is done, now start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        throw pjsua::Exception("Error starting pjsua", status);
    }

    /* Register to SIP server by creating SIP account. */
    pjsua_acc_config accConfig;

    pjsua_acc_config_default(&accConfig);

    accConfig.id = toPjString(string("sip:") + user + "@" + host);
    accConfig.reg_uri = toPjString(string("sip:") + host);

    accConfig.cred_count = 1;
    accConfig.cred_info[0].realm = toPjString(host);
    accConfig.cred_info[0].scheme = toPjString("digest");
    accConfig.cred_info[0].username = toPjString(user);
    accConfig.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    accConfig.cred_info[0].data = toPjString(password);

    pjsua_acc_id acc_id;
    status = pjsua_acc_add(&accConfig, PJ_TRUE, &acc_id);
    if (status != PJ_SUCCESS) {
        throw pjsua::Exception("Error adding account", status);
    }
}

pjsua::PjsuaCommunicator::~PjsuaCommunicator() {
    pjsua_destroy();
}

void pjsua::PjsuaCommunicator::loop() {

}
