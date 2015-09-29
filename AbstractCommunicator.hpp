#ifndef MUMSI_ABSTRACTCOMMUNICATOR_HPP
#define MUMSI_ABSTRACTCOMMUNICATOR_HPP

#include "SoundSampleQueue.hpp"
#include <stdint.h>

#define SOUND_SAMPLE_TYPE int16_t

class AbstractCommunicator {
public:

    virtual void loop() = 0;

protected:
    AbstractCommunicator(
            SoundSampleQueue<SOUND_SAMPLE_TYPE> &inputQueue,
            SoundSampleQueue<SOUND_SAMPLE_TYPE> &outputQueue)
            : inputQueue(inputQueue),
              outputQueue(outputQueue) { }

    SoundSampleQueue<SOUND_SAMPLE_TYPE> &inputQueue;
    SoundSampleQueue<SOUND_SAMPLE_TYPE> &outputQueue;
};

#endif //MUMSI_ABSTRACTCOMMUNICATOR_HPP
