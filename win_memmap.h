#pragma once

#include <windows.h>
#include <iostream>

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
        {
            UnmapViewOfFile(vptr);
        }
        void open_buffer_file(const char *fname)
        {
            HANDLE hFile = CreateFileA(fname,
                                       GENERIC_READ | GENERIC_WRITE, // dwDesiredAccess
                                       FILE_SHARE_READ,              // dwShareMode
                                       NULL,                         // lpSecurityAttributes
                                       CREATE_ALWAYS,                // dwCreationDisposition
                                       FILE_ATTRIBUTE_NORMAL,        // dwFlagsAndAttributes
                                       0);                           // hTemplateFile
            if (hFile == INVALID_HANDLE_VALUE)
            {
                return;
            }
            //-- the file is new and empty.
            // Set the eof to specified len.
            LARGE_INTEGER foo{.QuadPart = (LONGLONG)len};
            if (!SetFilePointerEx(hFile, foo, 0, FILE_BEGIN) ||
                !SetEndOfFile(hFile))
            {
                CloseHandle(hFile);
                std::cout << "Error: [" << GetLastError() << "] SetFilePointerEx() "
                                                             "could not extend the file "
                          << fname << " to " << len << " bytes.\n";
                return;
            }

            HANDLE hMap = CreateFileMappingA(hFile,
                                             NULL,           // Mapping attributes
                                             PAGE_READWRITE, // Protection flags
                                             0,   // MaximumSizeHigh
                                             0,    // MaximumSizeLow
                                             NULL);          // Name
            if (hMap == 0)
            {
                CloseHandle(hFile);
                return;
            }
            vptr = MapViewOfFile(hMap,
                                 FILE_MAP_READ | FILE_MAP_WRITE, // dwDesiredAccess
                                 0,                              // dwFileOffsetHigh
                                 0,                              // dwFileOffsetLow
                                 0);                             // dwNumberOfBytesToMap, entire map

            //-- Win, lose, or draw, we no longer need the
            // file or mapping handles. The mapped view will
            // hold the map and file open, or not.
            CloseHandle(hMap);
            CloseHandle(hFile);
        }
    };
    class MemoryMappedFile
    {
    private:
        void *vptr = 0;
        size_t len = 0;

    public:
        MemoryMappedFile(const char *fname)
        {
            open_file_map(fname);
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
        }
        void open_file_map(const char *fname)
        {
            HANDLE hFile = CreateFileA(fname,
                                       GENERIC_READ,          // dwDesiredAccess
                                       FILE_SHARE_READ,       // dwShareMode
                                       NULL,                  // lpSecurityAttributes
                                       OPEN_EXISTING,         // dwCreationDisposition
                                       FILE_ATTRIBUTE_NORMAL, // dwFlagsAndAttributes
                                       0);                    // hTemplateFile
            if (hFile == INVALID_HANDLE_VALUE)
            {
                DWORD err = GetLastError();
                return;
            }

            LARGE_INTEGER liFileSize;
            if (!GetFileSizeEx(hFile, &liFileSize) || liFileSize.QuadPart == 0)
            {
                DWORD err = GetLastError();
                CloseHandle(hFile);
                return;
            }
            len = liFileSize.QuadPart;

            HANDLE hMap = CreateFileMapping(hFile,
                                            NULL,          // Mapping attributes
                                            PAGE_READONLY, // Protection flags
                                            0,             // MaximumSizeHigh
                                            0,             // MaximumSizeLow
                                            NULL);         // Name
            if (hMap == 0)
            {
                DWORD err = GetLastError();
                CloseHandle(hFile);
                return;
            }

            vptr = MapViewOfFile(hMap,
                                 FILE_MAP_READ, // dwDesiredAccess
                                 0,             // dwFileOffsetHigh
                                 0,             // dwFileOffsetLow
                                 0);            // dwNumberOfBytesToMap, entire file.

            DWORD err = GetLastError();
            CloseHandle(hMap);
            CloseHandle(hFile);
        }
    };

} // namespace mhy