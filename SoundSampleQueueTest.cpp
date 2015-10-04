#include "SoundSampleQueue.hpp"

#include <boost/test/unit_test.hpp>

constexpr int CHUNK = 10;

BOOST_AUTO_TEST_CASE(SoundSampleQueueTest_Run) {

    SoundSampleQueue<int16_t> queue;

    int16_t buffer[CHUNK];

    int content = 0;
    for (int j = 0; j < 10; ++j) {
        for (int i = 0; i < CHUNK; ++i) {
            buffer[i] = content;
            content++;
        }
        queue.push_back(buffer, CHUNK);
    }

    for (int j = 0; j < 12; ++j) {
        int how_many = queue.pop_front(buffer, CHUNK);

        printf("%d: ", how_many);
        for (int i = 0; i < CHUNK; ++i) {
            printf("%d ", buffer[i]);
        }
        printf("\n");
    }

    BOOST_CHECK_EQUAL(1, 1);
}