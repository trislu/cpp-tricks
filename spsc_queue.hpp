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

    void push(T p)
    {
        /// NOTICE: it works as long as arithmetic on unsigned integers keep exhibiting modulo behavior.
        while ((uint8_t)(wpos.load() + 1) == rpos.load())
            ; // blocks when the buffer is full
        /// SEEALSO: https://cplusplus.com/reference/atomic/atomic/operatorplusplus/
        buffer[wpos++] = p;
    }

    T pop()
    {
        while (wpos.load() == rpos.load())
            ; // blocks when the buffer is full
        return buffer[rpos++];
    }

    bool try_push(const T &p)
    {
        if ((uint8_t)(wpos.load() + 1) == rpos.load())
            return false;
        buffer[wpos++] = p;
        return true;
    }

    bool try_pop(T &p)
    {
        if (wpos.load() == rpos.load())
            return false;
        p = buffer[rpos++];
        return false;
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

    void push(const T &ref)
    {
        /// NOTICE: it works as long as arithmetic on unsigned integers keep exhibiting modulo behavior.
        while (uint32_t(wpos.load() + 1) == rpos.load())
            ; // blocks when the buffer is full
        buffer[(wpos++) & CAPACITY_MASK] = ref;
    }

    T pop()
    {
        while (wpos.load() == rpos.load())
            ; // blocks when the buffer is empty
        return buffer[(rpos++) & CAPACITY_MASK];
    }

    bool try_push(const T &ref)
    {
        if (uint32_t(wpos.load() + 1) == rpos.load())
            return false;
        buffer[(wpos++) & CAPACITY_MASK] = ref;
        return true;
    }

    bool try_pop(T &out)
    {
        if (wpos.load() == rpos.load())
            return false;
        out = buffer[(rpos++) & CAPACITY_MASK];
        return true;
    }
};
