#pragma once

#ifdef WIN32
#include "win_memmap.h"
#else
//===============================================================
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace mhy {
class MemoryMappedFile
{
private:
    void *vptr = 0;
    size_t len = 0;

public:
    MemoryMappedFile(const char *fname)
    {   open_file_map(fname);
    }
    ~MemoryMappedFile()
    {   if (vptr) munmap(vptr, len);
    }

public:
    bool operator!() const
    {   return !vptr;
    }
    size_t size() const
    {   return len;
    }
    template <typename T>
    T *cast_to(size_t offset = 0)
    {
        if (offset >= len) return 0;
        return reinterpret_cast<T *>((char *)vptr + offset);
    }

private:
    void open_file_map(const char *fname)
    {
        int fd = open(fname, O_RDONLY);
        if (fd == -1)
        {   return;
        }
        struct stat sb;
        if (fstat(fd, &sb) == -1)
        {
            close(fd);
            return;
        }
        void *addr = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (addr == MAP_FAILED)
        {   return;
        }
        //----------------
        vptr = addr;
        len = sb.st_size;
    }
};

} // namespacee mhy
//===============================================================
#endif // not WIN32
