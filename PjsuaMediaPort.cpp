#include "PjsuaMediaPort.hpp"

using namespace std;
using namespace pjsua;


static pj_status_t getFrame(pjmedia_port *port,
                            pjmedia_frame *frame) {
    PjsuaMediaPort *mediaPort = static_cast<PjsuaMediaPort *>(port->port_data.pdata);
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);

    //todo code here
    for (int i = 0; i < count; ++i) {
        samples[i] = 10000.8 * (i % 10);
    }

    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    return PJ_SUCCESS;
}

static pj_status_t putFrame(pjmedia_port *port,
                            pjmedia_frame *frame) {
    PjsuaMediaPort *mediaPort = static_cast<PjsuaMediaPort *>(port->port_data.pdata);
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    pj_size_t count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);

    //todo code here

    return PJ_SUCCESS;
}

pjmedia_port *pjsua::PjsuaMediaPort::create_pjmedia_port() {

    _pjmedia_port = new pjmedia_port();

    pj_str_t name = toPjString("PjsuaMediaPort");

    pj_status_t status = pjmedia_port_info_init(&(_pjmedia_port->info),
                                                &name,
                                                PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                                                SAMPLING_RATE,
                                                1,
                                                16,
                                                SAMPLING_RATE * 20 / 1000); // todo recalculate to match mumble specs

    if (status != PJ_SUCCESS) {
        throw pjsua::Exception("Error while calling pjmedia_port_info_init().", status);
    }

    _pjmedia_port->get_frame = &getFrame;
    _pjmedia_port->put_frame = &putFrame;

    _pjmedia_port->port_data.pdata = this;

    return _pjmedia_port;
}

pjsua::PjsuaMediaPort::~PjsuaMediaPort() {
    if (this->_pjmedia_port != nullptr) {
        delete _pjmedia_port;
    }
}