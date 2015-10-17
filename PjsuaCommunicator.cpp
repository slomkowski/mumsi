#include "PjsuaCommunicator.hpp"


#include <pjlib.h>
#include <pjsua-lib/pjsua.h>

#include <functional>

#include <cstring>

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

sip::PjsuaCommunicator::PjsuaCommunicator(
        std::string host,
        std::string user,
        std::string password,
        unsigned int port) : logger(log4cpp::Category::getInstance("SipCommunicator")),
                             callbackLogger(log4cpp::Category::getInstance("SipCommunicatorCallback")) {

    pj_status_t status;

    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        throw sip::Exception("Error in pjsua_create()", status);
    }

    pjsua_config generalConfig;
    pjsua_config_default(&generalConfig);

    generalConfig.user_agent = toPjString("Mumsi Mumble-SIP Bridge");
    generalConfig.max_calls = 1;

    generalConfig.cb.on_incoming_call = &onIncomingCall;
    generalConfig.cb.on_call_media_state = &onCallMediaState;
    generalConfig.cb.on_call_state = &onCallState;

    //todo zrobić coś z logami
    pjsua_logging_config logConfig;
    pjsua_logging_config_default(&logConfig);

    logConfig.console_level = 4;

    status = pjsua_init(&generalConfig, &logConfig, NULL);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("Error in pjsua_init()", status);
    }

    pjsua_set_null_snd_dev();

    /* Add UDP transport. */
    pjsua_transport_config transportConfig;
    pjsua_transport_config_default(&transportConfig);

    transportConfig.port = port;

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transportConfig, NULL);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("Error creating transport", status);
    }

    pj_caching_pool cachingPool;
    pj_caching_pool_init(&cachingPool, &pj_pool_factory_default_policy, 0);
    pj_pool_t *pool = pj_pool_create(&cachingPool.factory, "wav", 32768, 8192, nullptr);

    // create circular buffers
    pjmedia_circ_buf_create(pool, 960 * 10, &inputBuff);
    pjmedia_circ_buf_create(pool, 960 * 10, &outputBuff);

    mediaPort = createMediaPort();

    pjsua_conf_add_port(pool, mediaPort, &mediaPortSlot);

    /* Initialization is done, now start sip */
    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        throw sip::Exception("Error starting sip", status);
    }

    registerAccount(host, user, password);
}

sip::PjsuaCommunicator::~PjsuaCommunicator() {
    pjsua_destroy();
}

pjmedia_port *sip::PjsuaCommunicator::createMediaPort() {

    pjmedia_port *mp = new pjmedia_port();

    pj_str_t name = toPjString("Pjsuamp");

    pj_status_t status = pjmedia_port_info_init(&(mp->info),
                                                &name,
                                                PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                                                SAMPLING_RATE,
                                                1,
                                                16,
                                                SAMPLING_RATE * 20 / 1000); // todo recalculate to match mumble specs

    if (status != PJ_SUCCESS) {
        throw sip::Exception("error while calling pjmedia_port_info_init().", status);
    }

    mp->get_frame = &MediaPort_getFrameRawCallback;
    mp->put_frame = &MediaPort_putFrameRawCallback;

    mp->port_data.pdata = this;

    return mp;
}

pj_status_t sip::MediaPort_getFrameRawCallback(pjmedia_port *port,
                                               pjmedia_frame *frame) {
    PjsuaCommunicator *communicator = static_cast<PjsuaCommunicator *>(port->port_data.pdata);
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    return communicator->mediaPortGetFrame(frame);
}

pj_status_t sip::MediaPort_putFrameRawCallback(pjmedia_port *port,
                                               pjmedia_frame *frame) {
    PjsuaCommunicator *communicator = static_cast<PjsuaCommunicator *>(port->port_data.pdata);
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    communicator->mediaPortPutFrame(samples, count);

    return PJ_SUCCESS;
}

pj_status_t sip::PjsuaCommunicator::mediaPortGetFrame(pjmedia_frame *frame) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&mediaPort->info);

    pj_size_t availableSamples = pjmedia_circ_buf_get_len(inputBuff);
    const int samplesToRead = std::min(count, availableSamples);

    callbackLogger.debug("Pulling %d samples from in-buff.", samplesToRead);
    pjmedia_circ_buf_read(inputBuff, samples, samplesToRead);

    if (availableSamples < count) {
        callbackLogger.debug("Requested %d samples, available %d, filling remaining with zeros.", count,
                             availableSamples);

        for (int i = samplesToRead; i < count; ++i) {
            samples[i] = 0;
        }
    }

    return PJ_SUCCESS;
}

void sip::PjsuaCommunicator::mediaPortPutFrame(pj_int16_t *samples, pj_size_t count) {
    std::unique_lock<std::mutex> lock(outBuffAccessMutex);

    callbackLogger.debug("Pushing %d samples to out-buff.", count);
    pjmedia_circ_buf_write(outputBuff, samples, count);

    lock.unlock();

    outBuffCondVar.notify_all();
}

void sip::PjsuaCommunicator::registerAccount(string host, string user, string password) {

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

    pj_status_t status = pjsua_acc_add(&accConfig, PJ_TRUE, &acc_id);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("failed to register account", status);
    }
}

void sip::PjsuaCommunicator::pushSamples(int16_t *samples, unsigned int length) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);
    callbackLogger.debug("Pushing %d samples to in-buff.", length);
    pjmedia_circ_buf_write(inputBuff, samples, length);
}

unsigned int sip::PjsuaCommunicator::pullSamples(int16_t *samples, unsigned int length, bool waitWhenEmpty) {
    std::unique_lock<std::mutex> lock(outBuffAccessMutex);

    unsigned int availableSamples;

    while ((availableSamples = pjmedia_circ_buf_get_len(inputBuff)) < length) {
        callbackLogger.debug("Not enough samples in buffer: %d, requested %d. Waiting.", availableSamples, length);
        outBuffCondVar.wait(lock);
    }

    const int samplesToRead = std::min(length, availableSamples);

    callbackLogger.debug("Pulling %d samples from out-buff.", samplesToRead);
    pjmedia_circ_buf_read(inputBuff, samples, samplesToRead);

    return samplesToRead;
}
