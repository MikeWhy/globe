#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include <memory>
#include <cstddef>


namespace mhy {
template <typename T>
class RangeT
{
public:
    typedef T*  PtrT;
    typedef T   value_type;
public:
    const PtrT first = 0;
    const PtrT last = 0;
public:
    RangeT() = default;
    RangeT(const RangeT<T> &) = default;
    RangeT(RangeT<T> &&) = default;
    template <typename U>
    RangeT(const RangeT<U> & u)
    : first((PtrT)u.first), last((PtrT)u.last)
    {}
    //-------
    RangeT(PtrT a, PtrT b)
    : first(a), last(b)
    {}
    RangeT(PtrT a, size_t count)
    : first(a), last(a + count)
    {}
    constexpr PtrT begin() const
    { return first;
    }
    constexpr PtrT end() const
    { return last;
    }
    constexpr size_t size() const
    { return last - first;
    }
};
template <typename T>
constexpr auto range(T* first, size_t count)
{   return RangeT<T>(first, count);
}
template <typename T>
constexpr auto range(T* first, T* last)
{   return RangeT<T>(first, last);
}
//========================
template <class T>
class ListT
{
public:
    typedef T   value_type;
    typedef T*  PtrT;
private:
    const RangeT<T> buf;
    PtrT            here;
public:
    ListT(RangeT<T> buf)
    : buf(buf), here(buf.begin())
    {}
    ListT(PtrT first, size_t count)
    : buf {first, count}, here(buf.begin())
    {}
    ~ListT()
    {}
public:
    bool operator!() const
    { return !remain();
    }
    bool empty() const
    { return here == begin();
    }
    constexpr PtrT begin() const
    { return buf.begin();
    }
    PtrT end() const
    { return here;
    }
    PtrT size() const
    { return here - begin();
    }
    size_t remain() const
    { return end() - here;
    }
    //-----
    void push_back(const T & val)
    {
        if (here == buf.end())
        { throw std::logic_error("List<T> insert past end of buffer.");
        }
        *here++ = val;
    }
    T& front()
    { return *begin();
    }
    const T& front() const 
    { return *begin();
    }
    T& back()
    { 
        //-- NO range checking.
        // if (empty())
        // { throw std::logic_error("List<T> read from empty list");
        // }
        return *(here - 1);
    }
    const T& back() const 
    { return *(here - 1);
    }
    T& operator[](size_t idx)
    { return *(begin() + idx);
    }
    const T& operator[](size_t idx) const
    { return *(begin() + idx);
    }
};

} // namespace