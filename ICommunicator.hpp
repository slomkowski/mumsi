#pragma once

#include <functional>
#include <inttypes.h>

class ICommunicator {
public:
    /**
     * Send samples through the Communicator.
     */
    virtual void sendPcmSamples(int16_t *samples, unsigned int length) = 0;

    /**
     * This callback is called when Communicator has received samples.
     */
    std::function<void(int16_t *, int)> onIncomingPcmSamples;
};
