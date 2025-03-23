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
#if 0
//-- TL;DR: Don't use this allocator for std::vector<>.
// std::vector<> also allocates internal housekeeping
// storage for itself out of your data pool.
//-- 
// So, you would think you could feed your cleverly
// pre-allocated memory map buffer to std::vector<> to
// contain your data. You say to the vector, "Here's a
// buffer. Put my data in it." What can be more natural?
//  Apparently, the vector thinks it just as natural
// and clever to place its own data in it, too. What
// data? Stuff to disambiguate its iterators. You'll
// recall this exercise began with invalidated iterators
// from vector's memory management.
//-- 
template <class T>
class MM_Allocator
{
public:
    typedef T               value_type;
    typedef std::ptrdiff_t  difference_type;
    typedef std::size_t     size_type;

public:
    const RangeT<T>     range;
    const std::string   my_name;
public:
    MM_Allocator() = default;
    MM_Allocator(const MM_Allocator<T> &) = default;
    MM_Allocator(MM_Allocator<T> &&) = default;

    template <typename U>
    MM_Allocator(const MM_Allocator<U> & u) 
    : range(u.range), my_name(u.my_name)
    {}

    MM_Allocator(RangeT<T> range, const std::string & whoami = {"anon"})
    : range(range), my_name(whoami)
    {}
    ~MM_Allocator()
    {}
public:
    constexpr T* allocate(size_t n)
    {   whoami(std::cout) << "allocate(" << n << ")\n";
        if (n > range.size()) throw std::bad_array_new_length();
        return range.begin();   // always working from this one buffer.
    }
    constexpr void deallocate(T* p, size_t n)
    { whoami(std::cout) << "deallocate(" << n << ")\n";
    }
    std::ostream & whoami(std::ostream & os) const
    { return os << "MM_Allocator[" << my_name << "]::";
    }

};
template <class T>
MM_Allocator<T> range_allocator(RangeT<T> range, const std::string & whoami)
{   return MM_Allocator<T>(range, whoami);
}
#endif

#if 0
void printMonitorInfo(GLFWmonitor* monitor)
{
    if (!monitor)
    {
        std::cerr << "Monitor is NULL" << std::endl;
        return;
    }

    int x, y; 
    glfwGetMonitorPos(monitor, &x, &y);

    int w, h;
    glfwGetMonitorPhysicalSize(monitor, &w, &h);

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    std::cout << "Monitor: " << glfwGetMonitorName(monitor) << std::endl;
    std::cout << "Position: " << x << ", " << y << std::endl;
    std::cout << "Physical size: " << w << 'x' << h << " mm" << std::endl;
    std::cout << "Video mode: " << mode->width << "x" << mode->height
                 << "@" << mode->refreshRate << "Hz" << std::endl;
    std::cout << std::endl;

    return;
}
GLFWmonitor* selectMonitor()
{
  if (!glfwInit()) {
    return nullptr;
  }
  int count = 0;
  auto monitors = glfwGetMonitors(&count);
  if (count == 0) {
    return nullptr;
  } else if (count == 1) {
    return monitors[0];
  }
  for (auto monitor : range(monitors, monitors + count)) {
    // Select monitor with resolution greater than 2560x1440.
    // On my system, the 4K monitor is connected to the discrete GPU.
    // The 2K monitors are on the integrated GPU. Later, the Vulkan
    // context will be created on Device connected to the selected monitor.
    // We want to select the larger, more capable discrete GPU.
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode->width > 2560 && mode->height > 1600) {
      return monitor;
    }
  }
  return nullptr;
}
#endif
} // namespace