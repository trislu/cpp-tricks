#include <assert.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include "spsc_queue.hpp"

void *producer(void *arg)
{
    spsc_queue_256<int> *q = (spsc_queue_256<int> *)arg;
    for (int i = 0; i < 500; i++)
    {
        q->push(i);
        std::cout << "pushed > " << i << std::endl;
    }
    return nullptr;
}

void *consumer(void *arg)
{
    sleep(10);
    spsc_queue_256<int> *q = (spsc_queue_256<int> *)arg;
    for (int i = 0; i < 500; i++)
    {
        int data = q->pop();
        assert(i == data);
        std::cout << "[" << i << "] popped = " << data << std::endl;
    }
    return nullptr;
}

int main(int argc, char **argv)
{
    spsc_queue_256<int> num_q;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_t foo;
    pthread_create(&foo, &attr, producer, &num_q);
    pthread_create(&foo, &attr, consumer, &num_q);

    for (;;)
    {
        sleep(2);
    }

    return 0;
}
