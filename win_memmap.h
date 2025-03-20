#pragma once

#include <windows.h>

namespace mhy
{
    class MemoryMappedFile
    {
    private:
        void *vptr = 0;
        size_t len = 0;
        HANDLE hFile = INVALID_HANDLE_VALUE;
        HANDLE hMap = INVALID_HANDLE_VALUE;
    public:
        MemoryMappedFile(const char *fname)
        { open_file_map(fname);
        }
        ~MemoryMappedFile()
        {
            if (vptr)
                close_file_map();
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
        void close_file_map()
        {
            UnmapViewOfFile(vptr);
            CloseHandle(hMap);
            CloseHandle(hFile);
        }
        void open_file_map(const char *fname)
        {
            hFile = CreateFileA(fname,
                                GENERIC_READ,          // dwDesiredAccess
                                FILE_SHARE_READ,       // dwShareMode
                                NULL,                  // lpSecurityAttributes
                                OPEN_EXISTING,         // dwCreationDisposition
                                FILE_ATTRIBUTE_NORMAL, // dwFlagsAndAttributes
                                0);                    // hTemplateFile
            if (hFile == INVALID_HANDLE_VALUE)
            { return;
            }

            LARGE_INTEGER liFileSize;
            if (!GetFileSizeEx(hFile, &liFileSize)
                || liFileSize.QuadPart == 0)
            {
                CloseHandle(hFile);
                return;
            }
            len = liFileSize.QuadPart;

            hMap = CreateFileMapping(
                hFile,
                NULL,          // Mapping attributes
                PAGE_READONLY, // Protection flags
                0,             // MaximumSizeHigh
                0,             // MaximumSizeLow
                NULL);         // Name
            if (hMap == 0)
            {
                CloseHandle(hFile);
                return;
            }

            vptr = MapViewOfFile(
                hMap,
                FILE_MAP_READ, // dwDesiredAccess
                0,             // dwFileOffsetHigh
                0,             // dwFileOffsetLow
                0);            // dwNumberOfBytesToMap
            if (vptr == NULL)
            {
                CloseHandle(hMap);
                CloseHandle(hFile);
                return;
            }
        }
    };

} // namespace mhy