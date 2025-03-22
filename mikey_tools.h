#pragma once
#include <iostream>

namespace mhy {
template <typename T>
class RangeT
{
private:
    T first;
    T last;
public:
    RangeT(T a, T b)
    : first(a), last(b)
    {}
    RangeT(T a, size_t count)
    : first(a), last(a + count)
    {}
    T begin() const
    { return first;
    }
    T end() const
    { return last;
    }
    size_t size() const
    { return last - first;
    }
};
template <typename T>
auto range(T first, size_t count)
{   return RangeT<T>(first, count);
}
template <typename T>
auto range(T first, T last)
{   return RangeT<T>(first, last);
}
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