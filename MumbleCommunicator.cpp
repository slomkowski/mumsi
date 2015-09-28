#include <cstring>
#include <functional>

#include "MumbleCommunicator.hpp"

void mumble_serversync_callback(char *welcome_text,
                                int32_t session,
                                int32_t max_bandwidth,
                                int64_t permissions,
                                void *userData) {
    printf("%s\n", welcome_text);
}

static int verify_cert(uint8_t *, uint32_t) {
    // Accept every cert
    return 1;
}

void mumble::MumbleCommunicator::receiveAudioFrameCallback(uint8_t *audio_data, uint32_t audio_data_size) {
    int dataPointer = 1;
    opus_int16 pcmData[1024];

    if (audio_data[0] == 0x80) {
        int64_t sessionId;
        int64_t sequenceNumber;
        int64_t opusDataLength;
        bool lastPacket;

        dataPointer += mumble_parse_variant(&sessionId, &audio_data[dataPointer]);
        dataPointer += mumble_parse_variant(&sequenceNumber, &audio_data[dataPointer]);
        dataPointer += mumble_parse_variant(&opusDataLength, &audio_data[dataPointer]);

        lastPacket = (opusDataLength & 0x2000) != 0;
        opusDataLength = opusDataLength & 0x1fff;


        unsigned int iAudioBufferSize;
        unsigned int iFrameSize = mumble::SAMPLE_RATE / 100;
        iAudioBufferSize = iFrameSize;
        iAudioBufferSize *= 12;
        int decodedSamples = opus_decode(opusState,
                                         reinterpret_cast<const unsigned char *>(&audio_data[dataPointer]),
                                         opusDataLength,
                                         pcmData,
                                         iAudioBufferSize,
                                         0);

        printf("\nsessionId: %ld, seqNum: %ld, opus data len: %ld, last: %d, pos: %ld, decoded: %d\n",
               sessionId, sequenceNumber, opusDataLength, lastPacket, dataPointer, decodedSamples);
    } else {
        printf("I received %d bytes of audio data\n", audio_data_size);
    }
}

static void mumble_audio_callback(uint8_t *audio_data, uint32_t audio_data_size, void *userData) {
    mumble::MumbleCommunicator *mumbleCommunicator = static_cast<mumble::MumbleCommunicator *>(userData);
    mumbleCommunicator->receiveAudioFrameCallback(audio_data, audio_data_size);
}


mumble::MumbleCommunicator::MumbleCommunicator(std::string user, std::string password, std::string host, int port) {

    opusState = opus_decoder_create(SAMPLE_RATE, 1, nullptr); //todo grab error

    struct mumble_config config;
    std::memset(&config, 0, sizeof(config));

    config.user_data = this;

    config.size = sizeof(config);
    config.host = const_cast<char *>(host.c_str());
    if (port > 0) {
        config.port = const_cast<char *>(std::to_string(port).c_str());
    }
    config.server_password = const_cast<char *>(password.c_str());
    config.username = const_cast<char *>(user.c_str());
    config.user_cert_filename = nullptr;
    config.user_privkey_filename = nullptr;

    config.ssl_verification_callback = verify_cert;
    config.audio_callback = mumble_audio_callback;
    config.serversync_callback = mumble_serversync_callback;

    mumble = mumble_connect(nullptr, &config);

    if (mumble == nullptr) {
        throw mumble::Exception("couldn't establish mumble connection");
    }
}

mumble::MumbleCommunicator::~MumbleCommunicator() {
    mumble_close(mumble);
}

void mumble::MumbleCommunicator::loop() {
    int quit = 0;
    while (quit == 0) {
        int status = mumble_tick(mumble);
        if (status < 0) {
            throw mumble::Exception("mumble_tick status " + status);
        }

        //todo Other processing here?
    }
}


