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
    //-- MemoryMappedFile is a read-only view of file content.
    // MappedBuffer is a writable file map.
    // An allocator class marries the mapped buffer to
    // std::vector's needs.
    //--
    class MappedBuffer
    {
    private:
        void *vptr = 0;
        const size_t len;

    public:
        MappedBuffer(const char *fname, size_t len)
            : len(len)
        {
            open_buffer_file(fname);
        }
        ~MappedBuffer()
        {
            if (vptr)
            {
                close_handles();
            }
        }

    public:
        bool operator!() const
        {
            return !vptr;
        }
        size_t size() const
        {
            return len;
        }
        template <typename T>
        T *cast_to(size_t offset = 0)
        {
            if (offset >= len)
                return 0;
            return reinterpret_cast<T *>((char *)vptr + offset);
        }

    private:
        void close_handles()
        {   if (vptr) munmap(vptr, len);
        }
        void open_buffer_file(const char *fname)
        {
            int fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0644);
            auto err = errno;
            if (fd == -1)
            {   return;
            }
            if (-1 == ftruncate64(fd, len) )
            {
                err = errno;
                close(fd);
                std::cout << "ERROR: ftruncatae(" << fname << ", " << len << ") "
                    "failed with errno " << err << std::endl;
                return;
            }
            void *addr = mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
            err = errno;
            close(fd);
            if (addr == MAP_FAILED)
            {
                std::cout << "ERROR: mmap() failed with errno " << err 
                    << " for file " << fname << ", " << len << " bytes.\n";
                return;
            }
            //----------------
            vptr = addr;
        }
    };
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
        struct stat64 sb;
        if (fstat64(fd, &sb) == -1)
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
