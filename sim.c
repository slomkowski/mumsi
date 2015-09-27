/* $Id: simple_pjsua.c 3553 2011-05-05 06:14:19Z nanang $ */
/*
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <pjsua-lib/pjsua.h>

#include <pjsip.h>
#include <pjmedia.h>
#include <pjlib.h>

#include <stdlib.h>     /* atoi() */
#include <stdio.h>
#include <math.h>       /* sin()  */

#define THIS_FILE "APP"

#define SIP_DOMAIN "sip.antisip.com"
#define SIP_USER "melangtone"
#define SIP_PASSWD "b8DU9AZXbd9tVCWg"


/* Struct attached to sine generator */
typedef struct {
} port_data;

pjmedia_port *sine_port;
int slot;

/* This callback is called to feed more samples */
static pj_status_t sine_get_frame(pjmedia_port *port,
                                  pjmedia_frame *frame) {
    port_data *sine = port->port_data.pdata;
    pj_int16_t *samples = frame->buf;
    pj_size_t count;

    if (PJMEDIA_PIA_CCNT(&port->info) != 1) {
        //todo throw exception
    }

    /* Get number of samples */
    count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);

    for (int i = 0; i < count; ++i) {
        samples[i] = 10000.8 * (i % 10);
    }

    /* Must set frame->type correctly, otherwise the sound device
     * will refuse to play.
     */
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

    return PJ_SUCCESS;
}

static pj_status_t sine_put_frame(pjmedia_port *port,
                                  pjmedia_frame *frame) {

    int sum = 0;
    pj_int16_t *samples = frame->buf;
    for (int i = 0; i < frame->size / 2; i++) {
        sum += samples[i];
    }
    printf("%d\n", sum);

    return PJ_SUCCESS;
}

/*
 * Create a media port to generate sine wave samples.
 */
static pj_status_t create_sine_port(pj_pool_t *pool,
                                    unsigned sampling_rate,
                                    unsigned channel_count,
                                    pjmedia_port **p_port) {
    pjmedia_port *port;
    unsigned i;
    unsigned count;
    pj_str_t name;
    port_data *sine;

    PJ_ASSERT_RETURN(pool && channel_count > 0 && channel_count <= 2, PJ_EINVAL);

    port = pj_pool_zalloc(pool, sizeof(pjmedia_port));
    PJ_ASSERT_RETURN(port != NULL, PJ_ENOMEM);

    /* Fill in port info. */
    name = pj_str("sine generator");
    pjmedia_port_info_init(&port->info, &name,
                           PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                           sampling_rate,
                           1,
                           16, sampling_rate * 20 / 1000 * 1);

    /* Set the function to feed frame */
    port->get_frame = &sine_get_frame;
    port->put_frame = &sine_put_frame;

    /* Create sine port data */
    port->port_data.pdata = sine = pj_pool_zalloc(pool, sizeof(port_data));

    *p_port = port;

    return PJ_SUCCESS;
}


/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
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

/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e) {
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &ci);
    PJ_LOG(3, (THIS_FILE, "Call %d state=%.*s", call_id,
            (int) ci.state_text.slen,
            ci.state_text.ptr));
}

/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id) {
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        // When media is active, connect call to sound device.
        pjsua_conf_connect(ci.conf_slot, slot);
        pjsua_conf_connect(slot, ci.conf_slot);
    }
}

/* Display error and exit application */
static void error_exit(const char *title, pj_status_t status) {
    pjsua_perror(THIS_FILE, title, status);
    pjsua_destroy();
    exit(1);
}

/*
 * main()
 *
 * argv[1] may contain URL to call.
 */
int main(int argc, char *argv[]) {
    pjsua_acc_id acc_id;
    pj_status_t status;
    pj_pool_t *pool;

    int channel_count = 1;

    /* Create pjsua first! */
    status = pjsua_create();
    if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);

    /* If argument is specified, it's got to be a valid SIP URL */
    if (argc > 1) {
        status = pjsua_verify_url(argv[1]);
        if (status != PJ_SUCCESS) error_exit("Invalid URL in argv", status);
    }

    /* Init pjsua */
    {
        pjsua_config cfg;
        pjsua_logging_config log_cfg;

        pjsua_config_default(&cfg);
        cfg.cb.on_incoming_call = &on_incoming_call;
        cfg.cb.on_call_media_state = &on_call_media_state;
        cfg.cb.on_call_state = &on_call_state;

        pjsua_logging_config_default(&log_cfg);
        log_cfg.console_level = 4;

        status = pjsua_init(&cfg, &log_cfg, NULL);
        if (status != PJ_SUCCESS) error_exit("Error in pjsua_init()", status);
    }

    /* Add UDP transport. */
    {
        pjsua_transport_config cfg;

        pjsua_transport_config_default(&cfg);
        cfg.port = 5060;
        status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
        if (status != PJ_SUCCESS) error_exit("Error creating transport", status);
    }

    pj_caching_pool cp;
    pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

    pool = pj_pool_create(&cp.factory,     /* pool factory         */
                          "wav",           /* pool name.           */
                          4000,            /* init size            */
                          4000,            /* increment size       */
                          NULL             /* callback on error    */
    );

    status = create_sine_port(pool,        /* memory pool          */
                              8000,       /* sampling rate        */
                              channel_count,/* # of channels       */
                              &sine_port   /* returned port        */
    );
    if (status != PJ_SUCCESS) {
        error_exit("Unable to create sine port", status);
        return 1;
    }

    pjsua_set_null_snd_dev();

    pjsua_conf_add_port(pool, sine_port, &slot);

    /* Initialization is done, now start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS) error_exit("Error starting pjsua", status);

    /* Register to SIP server by creating SIP account. */
    {
        pjsua_acc_config cfg;

        pjsua_acc_config_default(&cfg);
        cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
        cfg.reg_uri = pj_str("sip:" SIP_DOMAIN);
        cfg.cred_count = 1;
        cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
        cfg.cred_info[0].scheme = pj_str("digest");
        cfg.cred_info[0].username = pj_str(SIP_USER);
        cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
        cfg.cred_info[0].data = pj_str(SIP_PASSWD);

        status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
        if (status != PJ_SUCCESS) error_exit("Error adding account", status);
    }


    /* Wait until user press "q" to quit. */
    for (; ;) {
        char option[10];

        puts("Press 'h' to hangup all calls, 'q' to quit");
        if (fgets(option, sizeof(option), stdin) == NULL) {
            puts("EOF while reading stdin, will quit now..");
            break;
        }

        if (option[0] == 'q')
            break;

        if (option[0] == 'h')
            pjsua_call_hangup_all();
    }

    /* Destroy pjsua */
    pjsua_destroy();

    return 0;
}
