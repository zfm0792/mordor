// Copyright (c) 2009 - Decho Corp.

#include "mordor/pch.h"

#include "file.h"

#include "mordor/exception.h"
#include "mordor/string.h"

namespace Mordor {

FileStream::FileStream(const std::string &filename, AccessFlags accessFlags,
    CreateFlags createFlags, IOManager *ioManager, Scheduler *scheduler)
{
    NativeHandle handle;
#ifdef WINDOWS
    DWORD access = 0;
    if (accessFlags & READ)
        access |= GENERIC_READ;
    if (accessFlags & WRITE)
        access |= GENERIC_WRITE;
    if (accessFlags == APPEND)
        access = FILE_APPEND_DATA | SYNCHRONIZE;
    DWORD flags = 0;
    if (createFlags & DELETE_ON_CLOSE) {
        flags |= FILE_FLAG_DELETE_ON_CLOSE;
        createFlags = (CreateFlags)(createFlags & ~DELETE_ON_CLOSE);
    }
    if (ioManager)
        flags |= FILE_FLAG_OVERLAPPED;
    MORDOR_ASSERT(createFlags >= CREATE_NEW && createFlags <= TRUNCATE_EXISTING);
    handle = CreateFileW(toUtf16(filename).c_str(),
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        createFlags,
        flags,
        NULL);
    if (handle == INVALID_HANDLE_VALUE)
        MORDOR_THROW_EXCEPTION_FROM_LAST_ERROR_API("CreateFileW");
#else
    int oflags = (int)accessFlags;
    switch (createFlags & ~DELETE_ON_CLOSE) {
        case OPEN:
            break;
        case CREATE:
            oflags |= O_CREAT | O_EXCL;
            break;
        case OPEN_OR_CREATE:
            oflags |= O_CREAT;
            break;
        case OVERWRITE:
            oflags |= O_TRUNC;
            break;
        case OVERWRITE_OR_CREATE:
            oflags |= O_CREAT | O_TRUNC;
            break;
        default:
            MORDOR_NOTREACHED();
    }
    handle = open(filename.c_str(), oflags, 0777);
    if (handle < 0)
        MORDOR_THROW_EXCEPTION_FROM_LAST_ERROR_API("open");
    if (createFlags & DELETE_ON_CLOSE) {
        int rc = unlink(filename.c_str());
        if (rc != 0) {
            int error = errno;
            ::close(handle);
            MORDOR_THROW_EXCEPTION_FROM_ERROR_API(error, "unlink");
        }
    }
#endif       
    NativeStream::init(ioManager, scheduler, handle);
    m_supportsRead = accessFlags == READ || accessFlags == READWRITE;
    m_supportsWrite = accessFlags == WRITE || accessFlags == READWRITE ||
        accessFlags == APPEND;
    m_supportsSeek = accessFlags != APPEND;
}

#ifdef WINDOWS
FileStream::FileStream(const std::wstring &filename, AccessFlags accessFlags,
    CreateFlags createFlags, IOManager *ioManager, Scheduler *scheduler)
{
    NativeHandle handle;
    DWORD access = 0;
    if (accessFlags & READ)
        access |= GENERIC_READ;
    if (accessFlags & WRITE)
        access |= GENERIC_WRITE;
    if (accessFlags == APPEND)
        access = FILE_APPEND_DATA | SYNCHRONIZE;
    DWORD flags = 0;
    if (createFlags & DELETE_ON_CLOSE) {
        flags |= FILE_FLAG_DELETE_ON_CLOSE;
        createFlags = (CreateFlags)(createFlags & ~DELETE_ON_CLOSE);
    }
    if (ioManager)
        flags |= FILE_FLAG_OVERLAPPED;
    MORDOR_ASSERT(createFlags >= CREATE_NEW && createFlags <= TRUNCATE_EXISTING);
    handle = CreateFileW(filename.c_str(),
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        createFlags,
        flags,
        NULL);
    if (handle == INVALID_HANDLE_VALUE)
        MORDOR_THROW_EXCEPTION_FROM_LAST_ERROR_API("CreateFileW");
    NativeStream::init(ioManager, scheduler, handle);
    m_supportsRead = accessFlags == READ || accessFlags == READWRITE;
    m_supportsWrite = accessFlags == WRITE || accessFlags == READWRITE ||
        accessFlags == APPEND;
    m_supportsSeek = accessFlags != APPEND;
}
#endif

}
