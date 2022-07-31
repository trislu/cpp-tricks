#include <assert.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include "spsc_queue.hpp"

// typedef spsc_queue_po2<int, 8192> SQSC_QUEUE;
typedef spsc_queue_256<int> SQSC_QUEUE;

void *producer(void *arg)
{
    SQSC_QUEUE *q = (SQSC_QUEUE *)arg;
    for (int i = 0; i < 50000; i++)
    {
        // std::cout << "pushed > " << i << std::endl;
        q->push(i);
    }
    std::cout << "producer exit" << std::endl;
    return nullptr;
}

void *consumer(void *arg)
{
    // sleep(10);
    SQSC_QUEUE *q = (SQSC_QUEUE *)arg;
    for (int i = 0; i < 50000; i++)
    {
        int data;
        q->pop(data);
        if (i != data)
        {
            std::cout << "[" << i << "] mismatch: " << data << std::endl;
            abort();
        }
        // std::cout << "[" << i << "] popped = " << data << std::endl;
    }
    std::cout << "consumer exit" << std::endl;
    return nullptr;
}

int main(int argc, char **argv)
{
    SQSC_QUEUE num_q;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t foo, bar;
    pthread_create(&foo, &attr, producer, &num_q);
    pthread_create(&bar, &attr, consumer, &num_q);

    for (;;)
    {
        sleep(1);
    }
    return 0;
}
