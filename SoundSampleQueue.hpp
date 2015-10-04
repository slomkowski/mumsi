#ifndef MUMSI_SOUNDSAMPLEQUEUE_HPP
#define MUMSI_SOUNDSAMPLEQUEUE_HPP

#include <mutex>
#include <memory>
#include <cstring>
#include <algorithm>

template<typename SAMPLE_TYPE>
class SoundSampleQueue {
public:
    SoundSampleQueue()
            : start(0),
              stop(0) {
        buffer = new SAMPLE_TYPE[10000000];
    }

    ~SoundSampleQueue() {
        delete[] buffer;
    }

    void push_back(SAMPLE_TYPE *data, int length) {
        std::lock_guard<std::mutex> lock(accessMutex);

        for (int i = 0; i < length; ++i) {
            buffer[stop + i] = data[i];
        }

        stop += length;
    }

    int pop_front(SAMPLE_TYPE *data, int maxLength) {
        std::lock_guard<std::mutex> lock(accessMutex);

        int samplesToTake = std::min(stop - start, maxLength);

        for (int i = 0; i < samplesToTake; ++i) {
            data[i] = buffer[start + i];
        }
        start += samplesToTake;

        return samplesToTake;
    }

private:
    int start;
    int stop;

    std::mutex accessMutex;

    SAMPLE_TYPE *buffer;
};


#endif //MUMSI_SOUNDSAMPLEQUEUE_HPP
