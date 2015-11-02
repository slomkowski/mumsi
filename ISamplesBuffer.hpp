#ifndef MUMSI_ISAMPLESBUFFER_HPP
#define MUMSI_ISAMPLESBUFFER_HPP

#include <inttypes.h>

class ISamplesBuffer {
public:
    virtual void pushSamples(int16_t *samples, unsigned int length) = 0;

//    virtual unsigned int pullSamples(int16_t *samples, unsigned int length, bool waitWhenEmpty) = 0;
};


#endif //MUMSI_ISAMPLESBUFFER_HPP
