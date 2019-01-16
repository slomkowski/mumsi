#include "PjsuaCommunicator.hpp"

#include <pjlib.h>
#include <pjsua-lib/pjsua.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "main.hpp"

using namespace std;

namespace sip {
    using namespace log4cpp;

    class _LogWriter : public pj::LogWriter {
    public:
        _LogWriter(Category &logger)
                : logger(logger) { }

        virtual void write(const pj::LogEntry &entry) override {

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
        _MumlibAudioMedia(int call_id, sip::PjsuaCommunicator &comm, int frameTimeLength)
                : communicator(comm) {
            createMediaPort(call_id, frameTimeLength);
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

        void createMediaPort(int call_id, int frameTimeLength) {

            auto name = pj_str((char *) "MumsiMediaPort");

            if (frameTimeLength != 10
                and frameTimeLength != 20
                and frameTimeLength != 40
                and frameTimeLength != 60) {
                throw sip::Exception(
                        (boost::format("valid frame time length value: %d. valid values are: 10, 20, 40, 60") %
                         frameTimeLength).str());
            }

            pj_status_t status = pjmedia_port_info_init(&(mediaPort.info),
                                                        &name,
                                                        PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                                                        SAMPLING_RATE,
                                                        1,
                                                        16,
                                                        SAMPLING_RATE * frameTimeLength / 1000);

            if (status != PJ_SUCCESS) {
                throw sip::Exception("error while calling pjmedia_port_info_init()", status);
            }

            mediaPort.port_data.pdata = &communicator;
            // track call id in port_data
            mediaPort.port_data.ldata = (long) call_id;

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

        virtual void onCallState(pj::OnCallStateParam &prm) override;

        virtual void onCallMediaState(pj::OnCallMediaStateParam &prm) override;

        virtual void onDtmfDigit(pj::OnDtmfDigitParam &prm) override;

        virtual void playAudioFile(std::string file);
        virtual void playAudioFile(std::string file, bool in_chan);

    private:
        sip::PjsuaCommunicator &communicator;
        pj::Account &account;
    };

    class _Account : public pj::Account {
    public:
        _Account(sip::PjsuaCommunicator &comm, int max_calls)
                : communicator(comm) { this->max_calls = max_calls; }

        virtual void onRegState(pj::OnRegStateParam &prm) override;

        virtual void onIncomingCall(pj::OnIncomingCallParam &iprm) override;

    private:
        sip::PjsuaCommunicator &communicator;

        int active_calls = 0;
        int max_calls;

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

            // first, login to Mumble (only matters if MUM_DELAYED_CONNECT)
            communicator.calls[ci.id].onConnect();
            pj_thread_sleep(500); // sleep a moment to allow connection to stabilize

            communicator.logger.notice(msgText);
            communicator.calls[ci.id].sendUserStateStr(mumlib::UserState::COMMENT, msgText);
            communicator.calls[ci.id].onStateChange(msgText);

            pj_thread_sleep(500); // sleep a moment to allow connection to stabilize
            this->playAudioFile(communicator.file_welcome);

            communicator.got_dtmf = "";

            /* 
             * if no pin is set, go ahead and turn off mute/deaf
             * otherwise, wait for pin to be entered
             */
            if ( communicator.pins.size() == 0 ) {
                // No PIN set... enter DTMF root menu and turn off mute/deaf
                communicator.dtmf_mode = DTMF_MODE_ROOT;
                // turning off mute automatically turns off deaf
                communicator.calls[ci.id].sendUserState(mumlib::UserState::SELF_MUTE, false);
                pj_thread_sleep(500); // sleep a moment to allow connection to stabilize
                this->playAudioFile(communicator.file_announce_new_caller, true);
            } else {
                // PIN set... enter DTMF unauth menu and play PIN prompt message
                communicator.dtmf_mode = DTMF_MODE_UNAUTH;
                communicator.calls[ci.id].joinDefaultChannel();
                pj_thread_sleep(500); // pause briefly after announcement

                this->playAudioFile(communicator.file_prompt_pin);
            }

        } else if (ci.state == PJSIP_INV_STATE_DISCONNECTED) {
            auto &acc = dynamic_cast<_Account &>(account);

            /*
             * Not sure why we check acc.available, but with multi-call
             * functionality, this check doesn't work.
             */
            //if (not acc.available) {
                auto msgText = "Call from " + address + " finished.";

                communicator.calls[ci.id].mixer->clear();

                communicator.logger.notice(msgText);
                communicator.calls[ci.id].sendUserStateStr(mumlib::UserState::COMMENT, msgText);
                communicator.calls[ci.id].onStateChange(msgText);
                communicator.calls[ci.id].sendUserState(mumlib::UserState::SELF_DEAF, true);
                communicator.calls[ci.id].joinDefaultChannel();

                communicator.calls[ci.id].onDisconnect();

                //acc.available = true;
                acc.active_calls--;
            //}

            delete this;
        } else {
            communicator.logger.notice("MYDEBUG: unexpected state in onCallState() call:%d state:%d",
                    ci.id, ci.state);
        }
    }

    void _Call::onCallMediaState(pj::OnCallMediaStateParam &prm) {
        auto ci = getInfo();

        if (ci.media.size() != 1) {
            throw sip::Exception("ci.media.size is not 1");
        }

        if (ci.media[0].status == PJSUA_CALL_MEDIA_ACTIVE) {
            auto *aud_med = static_cast<pj::AudioMedia *>(getMedia(0));

            communicator.calls[ci.id].media->startTransmit(*aud_med);
            aud_med->startTransmit(*communicator.calls[ci.id].media);
        } else if (ci.media[0].status == PJSUA_CALL_MEDIA_NONE) {
            dynamic_cast<_Account &>(account).active_calls++;
        }
    }

    void _Call::playAudioFile(std::string file) {
        this->playAudioFile(file, false); // default is NOT to echo to mumble
    }

    /* TODO:
     * - local deafen before playing and undeafen after?
     */
    void _Call::playAudioFile(std::string file, bool in_chan) {
        communicator.logger.info("Entered playAudioFile(%s)", file.c_str());
        pj::AudioMediaPlayer player;
        pj::MediaFormatAudio mfa;
        pj::AudioMediaPlayerInfo pinfo;
        int wavsize;
        int sleeptime;

        if ( ! pj_file_exists(file.c_str()) ) {
            communicator.logger.warn("File not found (%s)", file.c_str());
            return;
        }

        /* TODO: use some library to get the actual length in millisec
         *
         * This just gets the file size and divides by a constant to
         * estimate the length of the WAVE file in milliseconds.
         * This depends on the encoding bitrate, etc.
         */

        auto ci = getInfo();
        if (ci.media.size() != 1) {
            throw sip::Exception("ci.media.size is not 1");
        }

        if (ci.media[0].status == PJSUA_CALL_MEDIA_ACTIVE) {
            auto *aud_med = static_cast<pj::AudioMedia *>(getMedia(0));

            try {
                player.createPlayer(file, PJMEDIA_FILE_NO_LOOP);
                pinfo = player.getInfo();
                sleeptime = (pinfo.sizeBytes / (pinfo.payloadBitsPerSample * 2.75));

                if ( in_chan ) { // choose the target sound output
                    player.startTransmit(*communicator.calls[ci.id].media);
                } else {
                    player.startTransmit(*aud_med);
                }

                pj_thread_sleep(sleeptime);

                if ( in_chan ) { // choose the target sound output
                    player.stopTransmit(*communicator.calls[ci.id].media);
                } else {
                    player.stopTransmit(*aud_med);
                }

            } catch (...) {
                communicator.logger.notice("Error playing file %s", file.c_str());
            }
        } else {
            communicator.logger.notice("Call not active - can't play file %s", file.c_str());
        }
    }

    void _Call::onDtmfDigit(pj::OnDtmfDigitParam &prm) {
        //communicator.logger.notice("DTMF digit '%s' (call %d).",
        //        prm.digit.c_str(), getId());
        pj::CallOpParam param;

        auto ci = getInfo();
        std::string chanName;

        /*
         * DTMF CALLER MENU
         */

        switch ( communicator.dtmf_mode ) {
            case DTMF_MODE_UNAUTH:
                /*
                 * IF UNAUTH, the only thing we allow is to authorize.
                 */
                switch ( prm.digit[0] ) {
                    case '#':
                        /*
                         * When user presses '#', test PIN entry
                         */
                        if ( communicator.pins.size() > 0 ) {
                            if ( communicator.pins.count(communicator.got_dtmf) > 0 ) {
                                communicator.logger.info("Caller entered correct PIN");
                                communicator.dtmf_mode = DTMF_MODE_ROOT;
                                communicator.logger.notice("MYDEBUG: %s:%s",
                                        communicator.got_dtmf.c_str(),
                                        communicator.pins[communicator.got_dtmf].c_str());
                                communicator.calls[ci.id].joinOtherChannel(
                                        communicator.pins[communicator.got_dtmf]);

                                this->playAudioFile(communicator.file_entering_channel);
                                communicator.calls[ci.id].sendUserState(mumlib::UserState::SELF_MUTE, false);
                                this->playAudioFile(communicator.file_announce_new_caller, true);
                            } else {
                                communicator.logger.info("Caller entered wrong PIN");
                                this->playAudioFile(communicator.file_invalid_pin);
                                if ( communicator.pin_fails++ >= MAX_PIN_FAILS ) {
                                param.statusCode = PJSIP_SC_SERVICE_UNAVAILABLE;
                                    pj_thread_sleep(500); // pause before next announcement
                                    this->playAudioFile(communicator.file_goodbye);
                                    pj_thread_sleep(500); // pause before next announcement
                                    this->hangup(param);
                                }
                                this->playAudioFile(communicator.file_prompt_pin);
                            }
                            communicator.got_dtmf = "";
                        }
                        break;
                    case '*':
                        /*
                         * Allow user to reset PIN entry by pressing '*'
                         */
                        communicator.got_dtmf = "";
                        this->playAudioFile(communicator.file_prompt_pin);
                        break;
                    default:
                        /* 
                         * In all other cases, add input digit to stack
                         */
                        communicator.got_dtmf = communicator.got_dtmf + prm.digit;
                        if ( communicator.got_dtmf.size() > MAX_CALLER_PIN_LEN ) {
                            // just drop 'em if too long
                            param.statusCode = PJSIP_SC_SERVICE_UNAVAILABLE;
                            this->playAudioFile(communicator.file_goodbye);
                            pj_thread_sleep(500); // pause before next announcement
                            this->hangup(param);
                        }
                }
                break;
            case DTMF_MODE_ROOT:
                /*
                 * User already authenticated; no data entry pending
                 */
                switch ( prm.digit[0] ) {
                    case '*':
                        /*
                         * Switch user to 'star' menu
                         */
                        communicator.dtmf_mode = DTMF_MODE_STAR;
                        break;
                    default:
                        /*
                         * Default is to ignore all digits in root
                         */
                        communicator.logger.info("Ignore DTMF digit '%s' in ROOT state", prm.digit.c_str());
                }
                break;
            case DTMF_MODE_STAR:
                /*
                 * User already entered '*'; time to perform action
                 */
                switch ( prm.digit[0] ) {
                    case '5':
                        // Mute line
                        communicator.calls[ci.id].sendUserState(mumlib::UserState::SELF_MUTE, true);
                        this->playAudioFile(communicator.file_mute_on);
                        break;
                    case '6':
                        // Un-mute line
                        this->playAudioFile(communicator.file_mute_off);
                        communicator.calls[ci.id].sendUserState(mumlib::UserState::SELF_MUTE, false);
                        break;
                    case '9':
                        if ( communicator.pins.size() > 0 ) {
                            communicator.dtmf_mode = DTMF_MODE_UNAUTH;
                            communicator.calls[ci.id].sendUserState(mumlib::UserState::SELF_DEAF, true);
                            communicator.calls[ci.id].joinDefaultChannel();
                            this->playAudioFile(communicator.file_prompt_pin);
                        } else {
                            // we should have a 'not supported' message
                        }
                        break;
                    case '0': // block these for the menu itself
                    case '*':
                    default:
                        // play menu
                        communicator.logger.info("Unsupported DTMF digit '%s' in state STAR", prm.digit.c_str());
                        this->playAudioFile(communicator.file_menu);
                        break;
                }
                /*
                 * In any case, switch back to root after one digit
                 */
                communicator.dtmf_mode = DTMF_MODE_ROOT;
                break;
            default:
                communicator.logger.info("Unexpected DTMF '%s' in unknown state '%d'", prm.digit.c_str(),
                        communicator.dtmf_mode);
        }

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

            if (active_calls < max_calls) {
                param.statusCode = PJSIP_SC_OK;
                active_calls++;
            } else {
                communicator.logger.notice("BUSY - reject incoming call from %s.", uri.c_str());
                param.statusCode = PJSIP_SC_OK;
                param.statusCode = PJSIP_SC_BUSY_EVERYWHERE;
            }

            call->answer(param);
        } else {
            communicator.logger.warn("Refusing call from %s.", uri.c_str());
            param.statusCode = PJSIP_SC_SERVICE_UNAVAILABLE;
            call->hangup(param);
        }
    }
}

sip::PjsuaCommunicator::PjsuaCommunicator(IncomingConnectionValidator &validator, int frameTimeLength, int maxCalls)
        : logger(log4cpp::Category::getInstance("SipCommunicator")),
          pjsuaLogger(log4cpp::Category::getInstance("Pjsua")),
          uriValidator(validator) {

    logWriter.reset(new sip::_LogWriter(pjsuaLogger));
    max_calls = maxCalls;


    endpoint.libCreate();

    pj::EpConfig endpointConfig;
    endpointConfig.uaConfig.userAgent = "Mumsi Mumble-SIP gateway";
    endpointConfig.uaConfig.maxCalls = maxCalls;

    endpointConfig.logConfig.writer = logWriter.get();
    endpointConfig.logConfig.level = 5;

    endpointConfig.medConfig.noVad = true;

    endpoint.libInit(endpointConfig);

    for(int i=0; i<maxCalls; ++i) {
        calls[i].index = i;
        pj_caching_pool_init(&(calls[i].cachingPool), &pj_pool_factory_default_policy, 0);
        calls[i].mixer.reset(new mixer::AudioFramesMixer(calls[i].cachingPool.factory));
        calls[i].media.reset(new _MumlibAudioMedia(i, *this, frameTimeLength));
    }

    logger.info("Created Pjsua communicator with frame length %d ms.", frameTimeLength);
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

void sip::PjsuaCommunicator::sendPcmSamples(int callId, int sessionId, int sequenceNumber, int16_t *samples, unsigned int length) {
    calls[callId].mixer->addFrameToBuffer(sessionId, sequenceNumber, samples, length);
}

pj_status_t sip::PjsuaCommunicator::mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame) {
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&(port->info));

    int call_id = (int) port->port_data.ldata;
    const int readSamples = calls[call_id].mixer->getMixedSamples(samples, count);

    if (readSamples < count) {
        pjsuaLogger.debug("Requested %d samples, available %d, filling remaining with zeros.",
                          count, readSamples);

        for (int i = readSamples; i < count; ++i) {
            samples[i] = 0;
        }
    }

    return PJ_SUCCESS;
}

pj_status_t sip::PjsuaCommunicator::mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame) {
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    int call_id = (int) port->port_data.ldata;

    if (count > 0) {
        pjsuaLogger.debug("Calling onIncomingPcmSamples with %d samples (call_id=%d).", count, call_id);
        this->calls[call_id].onIncomingPcmSamples(samples, count);
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
    account.reset(new _Account(*this, max_calls));
    account->create(accountConfig);
}

