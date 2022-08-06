
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

#include <cstdint>
#include <cstddef>
#include <array>
#include <atomic>

/** lock-free queue with fixed CAPACITY : 256 */
template <typename T>
class spsc_queue_256
{
private:
    spsc_queue_256(const spsc_queue_256 &);
    spsc_queue_256 &operator=(const spsc_queue_256 &);
    std::atomic_uint8_t wpos;
    std::atomic_uint8_t rpos;
    std::array<T, 256> buffer;

public:
    spsc_queue_256() : wpos(), rpos()
    {
        wpos.store(0);
        rpos.store(0);
    }

    std::size_t size()
    {
        /// NOTICE: it works as long as arithmetic on unsigned integers keep exhibiting modulo behavior.
        return wpos.load() - rpos.load();
    }

    void push(const T &elem)
    {
        // only the producer push can update write position
        uint8_t _w = wpos.load();
        // blocks when the buffer is full
        while (uint8_t(_w + 1) == rpos.load())
            ;
        // store elem firstly
        buffer[_w] = elem;
        // atomic move write position, so that consumer can detect
        wpos++;
    }

    void pop(T &out)
    {
        // only the consumer push can update read position
        uint8_t _r = rpos.load();
        // blocks when the buffer is empty
        while (wpos.load() == _r)
            ;
        // read out the element firstly
        out = buffer[_r];
        rpos++;
    }

    bool try_push(const T &elem)
    {
        // only the producer push can update write position
        uint8_t _w = wpos.load();
        // blocks when the buffer is full
        if (uint8_t(_w + 1) == rpos.load())
            return false;
        // store elem firstly
        buffer[_w] = elem;
        // atomic move write position, so that consumer can detect
        wpos++;
        return true;
    }

    bool try_pop(T &out)
    {
        // only the consumer push can update read position
        uint8_t _r = rpos.load();
        // blocks when the buffer is empty
        if (wpos.load() == _r)
            return false;
        // read out the element firstly
        out = buffer[_r];
        rpos++;
        return true;
    }
};

/** lock-free queue with fixed CAPACITY */
template <typename T, std::uint32_t CAPACITY>
class spsc_queue_po2
{
#define IS_POWER_OF_2(x) (0 == (x & (x - 1)))
    static_assert(IS_POWER_OF_2(CAPACITY), "CAPACITY must be a power of 2");
#undef IS_POWER_OF_2
private:
    spsc_queue_po2(const spsc_queue_po2 &);
    spsc_queue_po2 &operator=(const spsc_queue_po2 &);
    std::atomic_uint32_t wpos;
    std::atomic_uint32_t rpos;

    static const std::uint32_t CAPACITY_MASK = CAPACITY - 1;
    std::array<T, CAPACITY> buffer;

public:
    spsc_queue_po2() : wpos(0), rpos(0)
    {
    }

    std::size_t size()
    {
        uint32_t w = wpos.load();
        uint32_t r = rpos.load();
        return w > r ? w - r : r - w + CAPACITY;
    }

    void push(const T &elem)
    {
        /// NOTICE: it works as long as arithmetic on unsigned integers keep exhibiting modulo behavior.
        uint32_t _w = wpos.load();
        // blocks when the buffer is full
        while (uint32_t(_w - rpos.load()) == CAPACITY_MASK)
            ;
        buffer[_w & CAPACITY_MASK] = elem;
        wpos++;
    }

    void pop(T &out)
    {
        uint32_t _r = rpos.load();
        while (wpos.load() == _r)
            ; // blocks when the buffer is empty
        out = buffer[_r & CAPACITY_MASK];
        rpos++;
    }

    bool try_push(const T &elem)
    {
        uint32_t _w = wpos.load();
        while (uint32_t(_w - rpos.load()) == CAPACITY_MASK)
            return false;
        buffer[_w & CAPACITY_MASK] = elem;
        wpos++;
        return true;
    }

    bool try_pop(T &out)
    {
        uint32_t _r = rpos.load();
        if (wpos.load() == _r)
            return false; // blocks when the buffer is empty
        out = buffer[_r & CAPACITY_MASK];
        rpos++;
        return true;
    }
};
