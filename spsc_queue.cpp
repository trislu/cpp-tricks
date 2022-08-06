
/*
  Copyright (c) 2021 Lu Kai
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

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
