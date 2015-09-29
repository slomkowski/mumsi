#ifndef MUMSI_SOUNDSAMPLEQUEUE_HPP
#define MUMSI_SOUNDSAMPLEQUEUE_HPP

#include <mutex>
#include <memory>
#include <cstring>
#include <algorithm>

template<typename SAMPLE_TYPE>
class SoundSampleQueue {
public:
    SoundSampleQueue() {
        start = 0;
        stop = 0;
        buffer = new SAMPLE_TYPE[10000000];
    }

    ~SoundSampleQueue() {
        delete[] buffer;
    }

    void push(SAMPLE_TYPE *data, int length) {
        std::lock_guard<std::mutex> lock(accessMutex);

        std::memcpy(&buffer[stop], data, length);
        stop += length;
        // printf("pos: %d\n", stop);
    }

    int pop(SAMPLE_TYPE *data, int maxLength) {
        std::lock_guard<std::mutex> lock(accessMutex);

        int samplesToTake = std::min(stop - start, maxLength);
        std::memcpy(data, &buffer[stop - samplesToTake], samplesToTake);
        stop -= samplesToTake;

        //todo maksymalna pojemność bufora
        return samplesToTake;
    }

private:
    int start;
    int stop;

    std::mutex accessMutex;

    SAMPLE_TYPE *buffer;
};


#endif //MUMSI_SOUNDSAMPLEQUEUE_HPP
