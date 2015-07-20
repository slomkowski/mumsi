#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sndfile.hh>
#include <opus.h>
#include <pjsua-lib/pjsua.h>

extern "C" {
#include "libmumble.h"
}

unsigned int SAMPLE_RATE =48000;
OpusDecoder *opusState;
SndfileHandle fileHandle;

static int verify_cert(uint8_t *x509_pem_certificates, uint32_t size) {
    printf("I received a buffer of %d bytes!\n", size);
    const char FILENAME[] = "/tmp/cert";

    FILE *outfile = fopen(FILENAME, "wb");
    if (outfile != NULL) {
        fwrite(x509_pem_certificates, 1, size, outfile);
        fclose(outfile);
        printf("Written to %s\n", FILENAME);
    }

    // Accept every cert
    return 1;
}

void mumble_version_callback(uint16_t major, uint8_t minor, uint8_t patch, char *release, char *os, char *os_version) {
    printf("Version Callback: v%d.%d.%d. %s/%s/%s\n", major, minor, patch, release, os, os_version);
}

void mumble_audio_callback(uint8_t *audio_data, uint32_t audio_data_size) {
    int dataPointer = 1;
    float pcmData[1000];

    if (audio_data[0] == 0x80) {
        int64_t sessionId;
        int64_t sequenceNumber;
        int64_t opusDataLength;
        bool lastPacket = false;

        dataPointer += mumble_parse_variant(&sessionId, &audio_data[dataPointer]);
        dataPointer += mumble_parse_variant(&sequenceNumber, &audio_data[dataPointer]);
        dataPointer += mumble_parse_variant(&opusDataLength, &audio_data[dataPointer]);

        lastPacket = (opusDataLength & 0x2000) != 0;
        opusDataLength = opusDataLength & 0x1fff;


        unsigned int iAudioBufferSize;
        unsigned int iFrameSize =  SAMPLE_RATE / 100;
        iAudioBufferSize = iFrameSize;
        iAudioBufferSize *= 12;
        int decodedSamples = opus_decode_float(opusState,
                                           reinterpret_cast<const unsigned char *>(&audio_data[dataPointer]),
                                           opusDataLength,
                                           pcmData,
                                           iAudioBufferSize,
                                           0);

        printf("\nsessionId: %ld, seqNum: %ld, opus data len: %ld, last: %d, pos: %ld, decoded: %d\n",
               sessionId, sequenceNumber, opusDataLength, lastPacket, dataPointer, decodedSamples);

        fileHandle.write(pcmData, decodedSamples);
    }
    printf("I received %d bytes of audio data\n", audio_data_size);
}

void mumble_serversync_callback(char *welcome_text, int32_t session, int32_t max_bandwidth, int64_t permissions) {
    printf("%s\n", welcome_text);
}

void mumble_channelremove_callback(uint32_t channel_id) {
    printf("Channel %d removed\n", channel_id);
}

void mumble_channelstate_callback(char *name, int32_t channel_id, int32_t parent, char *description, uint32_t *links,
                                  uint32_t n_links, uint32_t *links_add, uint32_t n_links_add, uint32_t *links_remove,
                                  uint32_t n_links_remove, int32_t temporary, int32_t position) {
    printf("Obtained channel state\n");
}

void mumble_userremove_callback(uint32_t session, int32_t actor, char *reason, int32_t ban) {
    printf("User %d removed\n", actor);
}

void mumble_userstate_callback(int32_t session, int32_t actor, char *name, int32_t user_id, int32_t channel_id,
                               int32_t mute, int32_t deaf, int32_t suppress, int32_t self_mute, int32_t self_deaf,
                               char *comment, int32_t priority_speaker, int32_t recording) {
    printf("userstate_callback\n");
}

void mumble_banlist_callback(uint8_t *ip_data, uint32_t ip_data_size, uint32_t mask, char *name, char *hash,
                             char *reason, char *start, int32_t duration) {
    printf("banlist_callback\n");
}

void mumble_textmessage_callback(uint32_t actor, uint32_t n_session, uint32_t *session, uint32_t n_channel_id,
                                 uint32_t *channel_id, uint32_t n_tree_id, uint32_t *tree_id, char *message) {
    printf("textmessage_callback\n");
}

void mumble_permissiondenied_callback(int32_t permission, int32_t channel_id, int32_t session, char *reason,
                                      int32_t deny_type, char *name) {
    printf("permissiondenied_callback\n");
}

void mumble_acl_callback(void) {
    printf("acl_callback\n");
}

void mumble_queryusers_callback(uint32_t n_ids, uint32_t *ids, uint32_t n_names, char **names) {
    printf("queryusers_callback\n");
}

void mumble_cryptsetup_callback(uint32_t key_size, uint8_t *key, uint32_t client_nonce_size, uint8_t *client_nonce,
                                uint32_t server_nonce_size, uint8_t *server_nonce) {
    printf("cryptsetup_callback\n");
}

void mumble_contextactionmodify_callback(char *action, char *text, uint32_t m_context, uint32_t operation) {
    printf("contextactionmodify_callback\n");
}

void mumble_contextaction_callback(int32_t session, int32_t channel_id, char *action) {
    printf("contextaction_callback\n");
}

void mumble_userlist_callback(uint32_t user_id, char *name, char *last_seen, int32_t last_channel) {
    printf("userlist_callback\n");
}

void mumble_voicetarget_callback(void) {
    printf("voicetarget_callback\n");
}

void mumble_permissionquery_callback(int32_t channel_id, uint32_t permissions, int32_t flush) {
    printf("permissionquery_callback\n");
}

void mumble_codecversion_callback(int32_t alpha, int32_t beta, uint32_t prefer_alpha, int32_t opus) {
    printf("codecversion_callback\n");
}

void mumble_userstats_callback(void) {
    printf("userstats_callback\n");
}

void mumble_requestblob_callback(void) {
    printf("requestblob_callback\n");
}

void mumble_serverconfig_callback(uint32_t max_bandwidth, char *welcome_text, uint32_t allow_html,
                                  uint32_t message_length, uint32_t image_message_length) {
    printf("serverconfig_callback\n");
}

void mumble_suggestconfig_callback(uint32_t version, uint32_t positional, uint32_t push_to_talk) {
    printf("suggestconfig_callback\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mumble server ip> [mumble server port] [server password]\n", argv[0]);
        return -1;
    }

    char *host = argv[1];
    char *port = "64738";
    char *password = "hello";
    if (argc >= 3) {
        port = argv[2];
    }
    if (argc >= 4) {
        password = argv[3];
    }

    opusState = opus_decoder_create(SAMPLE_RATE, 1, nullptr);

    fileHandle = SndfileHandle("capture.wav", SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1, SAMPLE_RATE);

    struct mumble_config config;
    memset(&config, 0, sizeof(config));
    config.size = sizeof(config);
    config.host = host;
    config.port = port;
    config.server_password = password;
    config.username = "example_username";
    config.user_cert_filename = NULL;
    config.user_privkey_filename = NULL;
    config.ssl_verification_callback = verify_cert;

    config.version_callback = mumble_version_callback;
    config.audio_callback = mumble_audio_callback;
    config.serversync_callback = mumble_serversync_callback;
    config.channelremove_callback = mumble_channelremove_callback;
    config.channelstate_callback = mumble_channelstate_callback;
    config.userremove_callback = mumble_userremove_callback;
    config.userstate_callback = mumble_userstate_callback;
    config.banlist_callback = mumble_banlist_callback;
    config.textmessage_callback = mumble_textmessage_callback;
    config.permissiondenied_callback = mumble_permissiondenied_callback;
    config.acl_callback = mumble_acl_callback;
    config.queryusers_callback = mumble_queryusers_callback;
    config.cryptsetup_callback = mumble_cryptsetup_callback;
    config.contextactionmodify_callback = mumble_contextactionmodify_callback;
    config.contextaction_callback = mumble_contextaction_callback;
    config.userlist_callback = mumble_userlist_callback;
    config.voicetarget_callback = mumble_voicetarget_callback;
    config.permissionquery_callback = mumble_permissionquery_callback;
    config.codecversion_callback = mumble_codecversion_callback;
    config.userstats_callback = mumble_userstats_callback;
    config.requestblob_callback = mumble_requestblob_callback;
    config.serverconfig_callback = mumble_serverconfig_callback;
    config.suggestconfig_callback = mumble_suggestconfig_callback;

    struct mumble_struct *mumble = mumble_connect(NULL, &config);
    if (mumble == NULL) {
        printf("Couldn't establish mumble connection\n");
        return -1;
    }

    // Start processing here?

    int quit = 0;
    while (quit == 0) {
        if (mumble_tick(mumble) < 0) {
            quit = 1;
        }

        // Other processing here?
    }

    mumble_close(mumble);

    printf("Done\n");

    return 0;
}

