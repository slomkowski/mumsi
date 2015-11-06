#include "PjsuaCommunicator.hpp"

#include <pjlib.h>
#include <pjsua-lib/pjsua.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace std;

/**
 * These are global, because there's no way to pass it's value to onCallMediaState callback.
 */
//static int mediaPortSlot = -1;
//static sip::PjsuaCommunicator *pjsuaCommunicator = nullptr;

static log4cpp::Category &pjLogger = log4cpp::Category::getInstance("PjSip");


void sip::PjsuaCommunicator::onCallMediaState(pjsua_call_id call_id) {
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        pjsua_conf_connect(ci.conf_slot, mediaPortSlot);
        pjsua_conf_connect(mediaPortSlot, ci.conf_slot);
    }
}

void sip::PjsuaCommunicator::onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
    pjsua_call_info ci;

    PJ_UNUSED_ARG(rdata);

    pjsua_call_get_info(call_id, &ci);
    pjsua_call_set_user_data(call_id, this);

    logger.info("Incoming call from %s.", ci.remote_info.ptr);

    if (this->available) {
        available = false;

        pjsua_call_set_user_data(call_id, this);
        pjsua_call_answer(call_id, 200, nullptr, nullptr);
    } else {
        // 486 Busy Here - Callee is Busy
        pjsua_call_answer(call_id, 486, nullptr, nullptr);
    }
}

void sip::PjsuaCommunicator::onDtmfDigit(pjsua_call_id callId, int digit) {
    logger.notice("DTMF digit '%c' (call %d).", digit, callId);
}

void sip::PjsuaCommunicator::onCallState(pjsua_call_id call_id, pjsip_event *e) {
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &ci);

    logger.info("Call %d state=%s.", call_id, ci.state_text.ptr);

    string address = string(ci.remote_info.ptr);

    boost::replace_all(address, "<", "");
    boost::replace_all(address, ">", "");

    if (ci.state == PJSIP_INV_STATE_CONFIRMED) {
        auto msgText = "Start call from " + address + ".";

        logger.notice(msgText);
        onStateChange(msgText);
    } else if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
        auto msgText = "End call from " + address + ".";

        logger.notice(msgText);
        onStateChange(msgText);

        available = true;
    }
}

static void callback_onCallMediaState(pjsua_call_id callId) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(pjsua_call_get_user_data(callId));
    communicator->onCallMediaState(callId);
}

static void callback_onIncomingCall(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(pjsua_acc_get_user_data(acc_id));
    communicator->onIncomingCall(acc_id, call_id, rdata);
}

static void callback_onDtmfDigit(pjsua_call_id callId, int digit) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(pjsua_call_get_user_data(callId));
    communicator->onDtmfDigit(callId, digit);
}

static void callback_onCallState(pjsua_call_id call_id, pjsip_event *e) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(pjsua_call_get_user_data(call_id));
    communicator->onCallState(call_id, e);
}

static pj_status_t callback_getFrame(pjmedia_port *port, pjmedia_frame *frame) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
    return communicator->mediaPortGetFrame(port, frame);
}

static pj_status_t callback_putFrame(pjmedia_port *port, pjmedia_frame *frame) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
    return communicator->mediaPortPutFrame(port, frame);
}

static pj_status_t callback_onDestroy(pjmedia_port *port) {
    auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
    return communicator->_mediaPortOnDestroy(port);
}

static void pjLogToLog4CppBridgeFunction(int level, const char *data, int len) {
    using namespace log4cpp;
    std::map<int, Priority::Value> prioritiesMap = {
            {1, Priority::ERROR},
            {2, Priority::WARN},
            {3, Priority::NOTICE},
            {4, Priority::INFO},
            {5, Priority::DEBUG},
            {6, Priority::DEBUG}
    };

    string message(data);

    message = message.substr(0, message.size() - 1); // remove newline

    pjLogger << prioritiesMap.at(level) << message;
}

sip::PjsuaCommunicator::PjsuaCommunicator()
        : logger(log4cpp::Category::getInstance("SipCommunicator")),
          callbackLogger(log4cpp::Category::getInstance("SipCommunicatorCallback")) {
    pj_status_t status;


    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        throw sip::Exception("Error in pjsua_create()", status);
    }

    pj_log_set_log_func(pjLogToLog4CppBridgeFunction);

    pjsua_config generalConfig;
    pjsua_config_default(&generalConfig);

    string userAgent = "Mumsi Mumble-SIP Bridge";

    generalConfig.user_agent = toPjString(userAgent);
    generalConfig.max_calls = 1;

    generalConfig.cb.on_incoming_call = &callback_onIncomingCall;
    generalConfig.cb.on_dtmf_digit = &callback_onDtmfDigit;
    generalConfig.cb.on_call_media_state = &callback_onCallMediaState;
    generalConfig.cb.on_call_state = &callback_onCallState;

    pjsua_logging_config logConfig;
    pjsua_logging_config_default(&logConfig);
    logConfig.cb = pjLogToLog4CppBridgeFunction;
    logConfig.console_level = 5;

    pjsua_media_config mediaConfig;
    pjsua_media_config_default(&mediaConfig);
    mediaConfig.no_vad = 1;

    status = pjsua_init(&generalConfig, &logConfig, &mediaConfig);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error in pjsua_init", status);
    }

    pj_caching_pool cachingPool;
    pj_caching_pool_init(&cachingPool, &pj_pool_factory_default_policy, 0);
    pool = pj_pool_create(&cachingPool.factory, "wav", 32768, 8192, nullptr);

    // todo calculate sizes
    status = pjmedia_circ_buf_create(pool, 960 * 10, &inputBuff);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error when creating circular buffer", status);
    }

    logger.notice("Active ports: %d.", pjsua_conf_get_active_ports());

    createMediaPort();

    status = pjsua_conf_add_port(pool, &mediaPort, &mediaPortSlot);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error when calling pjsua_conf_add_port", status);
    }
}

void sip::PjsuaCommunicator::connect(
        std::string host,
        std::string user,
        std::string password,
        unsigned int port) {

    pj_status_t status;

    pjsua_transport_config transportConfig;
    pjsua_transport_config_default(&transportConfig);

    transportConfig.port = port;

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transportConfig, NULL);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error creating transport", status);
    }

    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error starting PJSUA", status);
    }

    status = pjsua_set_null_snd_dev();
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error in pjsua_set_null_std_dev()", status);
    }

    registerAccount(host, user, password);
}

sip::PjsuaCommunicator::~PjsuaCommunicator() {
    pjsua_destroy();
}

void sip::PjsuaCommunicator::createMediaPort() {

    eof = false;

    string name = "PjsuamediaPort";
    auto pjName = toPjString(name);

    pj_status_t status = pjmedia_port_info_init(&(mediaPort.info),
                                                &pjName,
                                                PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                                                SAMPLING_RATE,
                                                1,
                                                16,
                                                SAMPLING_RATE * 20 /
                                                1000); // todo recalculate to match mumble specs

    if (status != PJ_SUCCESS) {
        throw sip::Exception("error while calling pjmedia_port_info_init().", status);
    }

    mediaPort.port_data.pdata = this;

    mediaPort.get_frame = &callback_getFrame;
    mediaPort.put_frame = &callback_putFrame;
    mediaPort.on_destroy = &callback_onDestroy;
}

pj_status_t sip::PjsuaCommunicator::mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    if (this->eof) {
        return PJ_EEOF;
    }

    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&(port->info));

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

pj_status_t sip::PjsuaCommunicator::mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame) {
    if (this->eof) {
        return PJ_EEOF;
    }

    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    if (count > 0) {
        callbackLogger.debug("Calling onIncomingPcmSamples with %d samples.", count);
        onIncomingPcmSamples(samples, count);
    }

    return PJ_SUCCESS;
}

pj_status_t sip::PjsuaCommunicator::_mediaPortOnDestroy(pjmedia_port *port) {
    eof = true;
    return PJ_SUCCESS;
}

void sip::PjsuaCommunicator::registerAccount(string host, string user, string password) {

    pjsua_acc_config accConfig;
    pjsua_acc_config_default(&accConfig);

    string uri = string("sip:") + user + "@" + host;
    string regUri = "sip:" + host;
    string scheme = "digest";

    pj_status_t status;

    status = pjsua_verify_sip_url(uri.c_str());
    if (status != PJ_SUCCESS) {
        throw sip::Exception("invalid URI format", status);
    }

    logger.info("Registering account for URI: %s.", uri.c_str());

    accConfig.id = toPjString(uri);
    accConfig.reg_uri = toPjString(regUri);

    accConfig.cred_count = 1;
    accConfig.cred_info[0].realm = toPjString(host);
    accConfig.cred_info[0].scheme = toPjString(scheme);
    accConfig.cred_info[0].username = toPjString(user);
    accConfig.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    accConfig.cred_info[0].data = toPjString(password);
    accConfig.user_data = this;

    pjsua_acc_id acc_id;

    status = pjsua_acc_add(&accConfig, PJ_TRUE, &acc_id);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("failed to register account", status);
    }
}

void sip::PjsuaCommunicator::sendPcmSamples(int16_t *samples, unsigned int length) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);
    callbackLogger.debug("Pushing %d samples to in-buff.", length);
    pjmedia_circ_buf_write(inputBuff, samples, length);
}

