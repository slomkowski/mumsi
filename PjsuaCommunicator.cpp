#include "PjsuaCommunicator.hpp"

#include <pjlib.h>
#include <pjsua-lib/pjsua.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace std;

namespace sip {
    using namespace log4cpp;

    class _LogWriter : public pj::LogWriter {
    public:
        _LogWriter(Category &logger)
                : logger(logger) { }

        virtual void write(const pj::LogEntry &entry) {

            auto message = entry.msg.substr(0, entry.msg.size() - 1); // remove newline

            logger << prioritiesMap.at(entry.level) << message;
        }

    private:
        log4cpp::Category &logger;

        std::map<int, Priority::Value> prioritiesMap = {
                {1, Priority::ERROR},
                {2, Priority::WARN},
                {3, Priority::NOTICE},
                {4, Priority::INFO},
                {5, Priority::DEBUG},
                {6, Priority::DEBUG}
        };
    };

    class _MumlibAudioMedia : public pj::AudioMedia {
    public:
        _MumlibAudioMedia(sip::PjsuaCommunicator &comm)
                : communicator(comm) {
            createMediaPort();
            registerMediaPort(&mediaPort);
        }

        ~_MumlibAudioMedia() {
            unregisterMediaPort();
        }

    private:
        pjmedia_port mediaPort;
        sip::PjsuaCommunicator &communicator;

        static pj_status_t callback_getFrame(pjmedia_port *port, pjmedia_frame *frame) {
            auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
            return communicator->mediaPortGetFrame(port, frame);
        }

        static pj_status_t callback_putFrame(pjmedia_port *port, pjmedia_frame *frame) {
            auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
            return communicator->mediaPortPutFrame(port, frame);
        }

        void createMediaPort() {

            auto name = pj_str((char *) "MumsiMediaPort");

            pj_status_t status = pjmedia_port_info_init(&(mediaPort.info),
                                                        &name,
                                                        PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                                                        SAMPLING_RATE,
                                                        1,
                                                        16,
                                                        SAMPLING_RATE * 20 /
                                                        1000); // todo recalculate to match mumble specs

            if (status != PJ_SUCCESS) {
                throw sip::Exception("error while calling pjmedia_port_info_init()", status);
            }

            mediaPort.port_data.pdata = &communicator;

            mediaPort.get_frame = &callback_getFrame;
            mediaPort.put_frame = &callback_putFrame;
        }
    };

    class _Call : public pj::Call {
    public:
        _Call(sip::PjsuaCommunicator &comm, pj::Account &acc, int call_id = PJSUA_INVALID_ID)
                : pj::Call(acc, call_id),
                  communicator(comm),
                  account(acc) { }

        virtual void onCallState(pj::OnCallStateParam &prm);

        virtual void onCallMediaState(pj::OnCallMediaStateParam &prm);

        virtual void onDtmfDigit(pj::OnDtmfDigitParam &prm);

    private:
        sip::PjsuaCommunicator &communicator;
        pj::Account &account;
    };

    class _Account : public pj::Account {
    public:
        _Account(sip::PjsuaCommunicator &comm)
                : communicator(comm) { }

        virtual void onRegState(pj::OnRegStateParam &prm);

        virtual void onIncomingCall(pj::OnIncomingCallParam &iprm);

    private:
        sip::PjsuaCommunicator &communicator;

        bool available = true;

        friend class _Call;
    };

    void _Call::onCallState(pj::OnCallStateParam &prm) {
        auto ci = getInfo();

        communicator.logger.info("Call %d state=%s.", ci.id, ci.stateText.c_str());

        string address = ci.remoteUri;

        boost::replace_all(address, "<", "");
        boost::replace_all(address, ">", "");

        if (ci.state == PJSIP_INV_STATE_CONFIRMED) {
            auto msgText = "Incoming call from " + address + ".";

            communicator.logger.notice(msgText);
            communicator.onStateChange(msgText);
        } else if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
            auto &acc = dynamic_cast<_Account &>(account);

            if (not acc.available) {
                auto msgText = "Call from " + address + " finished.";

                communicator.logger.notice(msgText);
                communicator.onStateChange(msgText);

                acc.available = true;
            }

            delete this;
        }
    }

    void _Call::onCallMediaState(pj::OnCallMediaStateParam &prm) {
        auto ci = getInfo();

        if (ci.media.size() != 1) {
            throw sip::Exception("ci.media.size is not 1");
        }

        if (ci.media[0].status == PJSUA_CALL_MEDIA_ACTIVE) {
            auto *aud_med = static_cast<pj::AudioMedia *>(getMedia(0));

            communicator.media->startTransmit(*aud_med);
            aud_med->startTransmit(*communicator.media);
        } else if (ci.media[0].status == PJSUA_CALL_MEDIA_NONE) {
            dynamic_cast<_Account &>(account).available = true;
        }
    }

    void _Call::onDtmfDigit(pj::OnDtmfDigitParam &prm) {
        communicator.logger.notice("DTMF digit '%s' (call %d).", prm.digit.c_str(), getId());
    }

    void _Account::onRegState(pj::OnRegStateParam &prm) {
        pj::AccountInfo ai = getInfo();
        communicator.logger << log4cpp::Priority::INFO
        << (ai.regIsActive ? "Register:" : "Unregister:") << " code=" << prm.code;
    }

    void _Account::onIncomingCall(pj::OnIncomingCallParam &iprm) {
        auto *call = new _Call(communicator, *this, iprm.callId);

        string uri = call->getInfo().remoteUri;

        communicator.logger.info("Incoming call from %s.", uri.c_str());

        pj::CallOpParam param;

        if (communicator.uriValidator.validateUri(uri)) {

            if (available) {
                param.statusCode = PJSIP_SC_OK;
                available = false;
            } else {
                param.statusCode = PJSIP_SC_BUSY_EVERYWHERE;
            }

            call->answer(param);
        } else {
            communicator.logger.warn("Refusing call from %s.", uri.c_str());
            param.statusCode = PJSIP_SC_DECLINE;
            call->hangup(param);
        }
    }
}

sip::PjsuaCommunicator::PjsuaCommunicator(IncomingConnectionValidator &validator)
        : logger(log4cpp::Category::getInstance("SipCommunicator")),
          pjsuaLogger(log4cpp::Category::getInstance("Pjsua")),
          uriValidator(validator) {

    logWriter.reset(new sip::_LogWriter(pjsuaLogger));

    endpoint.libCreate();

    pj::EpConfig endpointConfig;
    endpointConfig.uaConfig.userAgent = "Mumsi Mumble-SIP gateway";
    endpointConfig.uaConfig.maxCalls = 1;

    endpointConfig.logConfig.writer = logWriter.get();
    endpointConfig.logConfig.level = 5;

    endpointConfig.medConfig.noVad = true;

    endpoint.libInit(endpointConfig);

    pj_status_t status;
    pj_caching_pool cachingPool;
    pj_caching_pool_init(&cachingPool, &pj_pool_factory_default_policy, 0);
    pool = pj_pool_create(&cachingPool.factory, "media", 32768, 8192, nullptr);
    if (!pool) {
        throw sip::Exception("error when creating memory pool", status);
    }

    // todo calculate sizes
    status = pjmedia_circ_buf_create(pool, 960 * 10, &inputBuff);
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error when creating circular buffer", status);
    }

    media.reset(new _MumlibAudioMedia(*this));
}

void sip::PjsuaCommunicator::connect(
        std::string host,
        std::string user,
        std::string password,
        unsigned int port) {

    pj::TransportConfig transportConfig;

    transportConfig.port = port;

    endpoint.transportCreate(PJSIP_TRANSPORT_UDP, transportConfig); // todo try catch

    endpoint.libStart();

    pj_status_t status = pjsua_set_null_snd_dev();
    if (status != PJ_SUCCESS) {
        throw sip::Exception("error in pjsua_set_null_std_dev()", status);
    }

    registerAccount(host, user, password);
}

sip::PjsuaCommunicator::~PjsuaCommunicator() {
    endpoint.libDestroy();
}


pj_status_t sip::PjsuaCommunicator::mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&(port->info));

    pj_size_t availableSamples = pjmedia_circ_buf_get_len(inputBuff);
    const int samplesToRead = std::min(count, availableSamples);

    pjsuaLogger.debug("Pulling %d samples from in-buff.", samplesToRead);
    pjmedia_circ_buf_read(inputBuff, samples, samplesToRead);

    if (availableSamples < count) {
        pjsuaLogger.debug("Requested %d samples, available %d, filling remaining with zeros.", count,
                          availableSamples);

        for (int i = samplesToRead; i < count; ++i) {
            samples[i] = 0;
        }
    }

    return PJ_SUCCESS;
}

pj_status_t sip::PjsuaCommunicator::mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame) {
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    if (count > 0) {
        pjsuaLogger.debug("Calling onIncomingPcmSamples with %d samples.", count);
        onIncomingPcmSamples(samples, count);
    }

    return PJ_SUCCESS;
}

void sip::PjsuaCommunicator::registerAccount(string host, string user, string password) {

    string uri = "sip:" + user + "@" + host;
    pj::AccountConfig accountConfig;
    accountConfig.idUri = uri;
    accountConfig.regConfig.registrarUri = "sip:" + host;

    pj::AuthCredInfo cred("digest", "*", user, 0, password);
    accountConfig.sipConfig.authCreds.push_back(cred);

    logger.info("Registering account for URI: %s.", uri.c_str());
    account.reset(new _Account(*this));
    account->create(accountConfig);
}

void sip::PjsuaCommunicator::sendPcmSamples(int16_t *samples, unsigned int length) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);
    pjsuaLogger.debug("Pushing %d samples to in-buff.", length);
    pjmedia_circ_buf_write(inputBuff, samples, length);
}

