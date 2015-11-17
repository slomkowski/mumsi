#include "AudioFramesMixer.hpp"


//// Reset mix buffer.
//Arrays.fill(tempMix, 0);
//
//// Sum the buffers.
//for (final AudioUser user : mix) {
//for (int i = 0; i < tempMix.length; i++) {
//tempMix[i] += user.lastFrame[i];
//}
//}
//
//// Clip buffer for real output.
//for (int i = 0; i < MumbleProtocol.FRAME_SIZE; i++) {
//clipOut[i] = (short) (Short.MAX_VALUE * (tempMix[i] < -1.0f ? -1.0f
//: (tempMix[i] > 1.0f ? 1.0f : tempMix[i])));
//}

mixer::AudioFramesMixer::AudioFramesMixer(pj_pool_factory &poolFactory)
        : logger(log4cpp::Category::getInstance("AudioFramesMixer")) {

    pool = pj_pool_create(&poolFactory, "media", 32768, 8192, nullptr);
    if (!pool) {
        throw mixer::Exception("error when creating memory pool");
    }

    // todo calculate sizes
    pj_status_t status = pjmedia_circ_buf_create(pool, 960 * 10, &inputBuff);
    if (status != PJ_SUCCESS) {
        throw mixer::Exception("error when creating circular buffer", status);
    }
}

mixer::AudioFramesMixer::~AudioFramesMixer() {
    if (pool != nullptr) {
        pj_pool_release(pool);
    }
}

void mixer::AudioFramesMixer::addFrameToBuffer(int sessionId, int sequenceNumber, int16_t *samples, int samplesLength) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);
    logger.debug("Pushing %d samples to in-buff.", samplesLength);
    pjmedia_circ_buf_write(inputBuff, samples, samplesLength);
}

int mixer::AudioFramesMixer::getMixedSamples(int16_t *mixedSamples, int requestedLength) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    int availableSamples = pjmedia_circ_buf_get_len(inputBuff);
    const int samplesToRead = std::min(requestedLength, availableSamples);

    logger.debug("Pulling %d samples from in-buff.", samplesToRead);
    pjmedia_circ_buf_read(inputBuff, mixedSamples, samplesToRead);

    return samplesToRead;
}

void mixer::AudioFramesMixer::clean() {

}
