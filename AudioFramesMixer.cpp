#include "AudioFramesMixer.hpp"

#include <boost/format.hpp>

#include <climits>

mixer::AudioFramesMixer::AudioFramesMixer(pj_pool_factory &poolFactory)
        : logger(log4cpp::Category::getInstance("AudioFramesMixer")) {

    pool = pj_pool_create(&poolFactory, "mixer_pool", 10 * 1024, 10 * 1024, nullptr);
    if (!pool) {
        throw mixer::Exception("error when creating memory pool");
    }
}

mixer::AudioFramesMixer::~AudioFramesMixer() {
    if (pool != nullptr) {
        pj_pool_release(pool);
    }
}

void mixer::AudioFramesMixer::addFrameToBuffer(int sessionId, int sequenceNumber, int16_t *samples, int samplesLength) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    pjmedia_circ_buf *circBuf;
    pj_status_t status;

    auto it = buffersMap.find(sessionId);

    if (it != buffersMap.end()) {
        circBuf = it->second;
    } else {
        logger.debug("Creating circular buffer for session %d.", sessionId);
        status = pjmedia_circ_buf_create(pool, 960 * 10, &circBuf);
        if (status != PJ_SUCCESS) {
            throw mixer::Exception("error when creating circular buffer", status);
        }
        buffersMap.insert({{sessionId, circBuf}});
    }

    logger.debug("Pushing %d samples to buffer for session %d.", samplesLength, sessionId);
    status = pjmedia_circ_buf_write(circBuf, samples, samplesLength);
    if (status != PJ_SUCCESS and status != PJ_ETOOBIG) {
        throw mixer::Exception((boost::format("error when writing %d samples to circular buffer")
                                % samplesLength).str(), status);
    }
}

int mixer::AudioFramesMixer::getMixedSamples(int16_t *mixedSamples, int requestedLength) {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    double mixerBuffer[MAX_BUFFER_LENGTH];
    memset(mixerBuffer, 0, sizeof(mixerBuffer));

    int longestSamples = 0;

    for (auto &user: buffersMap) {
        int16_t userBuff[MAX_BUFFER_LENGTH];

        int availableSamples = pjmedia_circ_buf_get_len(user.second);
        const int samplesToRead = std::min(requestedLength, availableSamples);

        longestSamples = std::max(samplesToRead, longestSamples);

        logger.debug("Pulling %d samples from in-buff for session ID %d.", samplesToRead, user.first);

        pj_status_t status = pjmedia_circ_buf_read(user.second, userBuff, samplesToRead);
        if (status != PJ_SUCCESS) {
            throw mixer::Exception(
                    (boost::format("error when pulling %d samples from buffer for session ID %d")
                     % samplesToRead % user.first).str(), status);
        }

        for (int i = 0; i < samplesToRead; ++i) {
            mixerBuffer[i] += userBuff[i];
        }
    }

    for (auto it = buffersMap.cbegin(); it != buffersMap.cend() /* not hoisted */; /* no increment */) {
        if (pjmedia_circ_buf_get_len(it->second) == 0) {
            logger.debug("Removing circular buffer for session %d.", it->first);
            pj_status_t status = pjmedia_circ_buf_reset(it->second);
            if (status != PJ_SUCCESS) {
                throw mixer::Exception(
                        (boost::format("error when resetting circular buffer for session ID %d") % it->first).str(),
                        status);
            }
            buffersMap.erase(it++);
        }
        else {
            ++it;
        }
    }

    double maxVal = 0;
    for (int i = 0; i < longestSamples; ++i) {
        maxVal = std::max(maxVal, std::abs(mixerBuffer[i]));
    }

    if (maxVal >= INT16_MAX) {
        for (int i = 0; i < longestSamples; ++i) {
            mixedSamples[i] = (INT16_MAX * (mixerBuffer[i] / maxVal));
        }
        logger.debug("Mixer overdrive, truncating to 16-bit.");
    } else {
        for (int i = 0; i < longestSamples; ++i) {
            mixedSamples[i] = mixerBuffer[i];
        }
    }

    logger.debug("Getting %d mixed samples.", longestSamples);

    return longestSamples;
}

void mixer::AudioFramesMixer::clear() {
    std::unique_lock<std::mutex> lock(inBuffAccessMutex);

    for (auto &entry : buffersMap) {
        pjmedia_circ_buf_reset(entry.second);
    }

    buffersMap.clear();
}
