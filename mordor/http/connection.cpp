// Copyright (c) 2009 - Decho Corp.

#include "mordor/pch.h"

#include "connection.h"

#include "chunked.h"
#include "mordor/streams/buffered.h"
#include "mordor/streams/gzip.h"
#include "mordor/streams/limited.h"
#include "mordor/streams/notify.h"
#include "mordor/streams/singleplex.h"
#include "mordor/streams/zlib.h"

namespace Mordor {
namespace HTTP {

Connection::Connection(Stream::ptr stream)
: m_stream(stream)
{
    MORDOR_ASSERT(stream);
    MORDOR_ASSERT(stream->supportsRead());
    MORDOR_ASSERT(stream->supportsWrite());
    if (!stream->supportsUnread() || !stream->supportsFind()) {
        BufferedStream *buffered = new BufferedStream(stream);
        buffered->allowPartialReads(true);
        m_stream.reset(buffered);
    }
}

bool
Connection::hasMessageBody(const GeneralHeaders &general,
                                 const EntityHeaders &entity,
                                 Method method,
                                 Status status)
{
    if (status == INVALID) {
        // Request
        switch (method) {
            case GET:
            case HEAD:
            case TRACE:
            case CONNECT:
                return false;
            default:
                break;
        }
        if (entity.contentLength != ~0ull && entity.contentLength != 0)
            return true;
        if (entity.contentLength == 0)
            return false;
        for (ParameterizedList::const_iterator it(general.transferEncoding.begin());
            it != general.transferEncoding.end();
            ++it) {
            if (stricmp(it->value.c_str(), "identity") != 0)
                return true;
        }
        if (entity.contentType.type == "multipart")
            return true;
        throw std::runtime_error("Requests must have some way to determine when they end!");
    } else {
        // Response
        switch (method) {
            case HEAD:
            case TRACE:
                return false;
            default:
                break;
        }
        if (((int)status >= 100 && status <= 199) ||
            (int)status == 204 ||
            (int)status == 304 ||
            method == HEAD)
            return false;
        if ((int)status == 200 && method == CONNECT)
            return false;
        for (ParameterizedList::const_iterator it(general.transferEncoding.begin());
            it != general.transferEncoding.end();
            ++it) {
            if (stricmp(it->value.c_str(), "identity") != 0)
                return true;
        }
        if (entity.contentLength == 0)
            return false;
        if (entity.contentType.type == "multipart")
            return true;
        return true;
    }
}

Stream::ptr
Connection::getStream(const GeneralHeaders &general,
                            const EntityHeaders &entity,
                            Method method,
                            Status status,
                            boost::function<void()> notifyOnEof,
                            boost::function<void()> notifyOnException,
                            bool forRead)
{
    MORDOR_ASSERT(hasMessageBody(general, entity, method, status));
    Stream::ptr stream;
    if (forRead) {
        stream.reset(new SingleplexStream(m_stream, SingleplexStream::READ, false));
    } else {
        stream.reset(new SingleplexStream(m_stream, SingleplexStream::WRITE, false));
    }
    Stream::ptr baseStream(stream);
    for (ParameterizedList::const_reverse_iterator it(general.transferEncoding.rbegin());
        it != general.transferEncoding.rend();
        ++it) {
        if (stricmp(it->value.c_str(), "chunked") == 0) {
            stream.reset(new ChunkedStream(stream));
        } else if (stricmp(it->value.c_str(), "deflate") == 0) {
            stream.reset(new ZlibStream(stream));
        } else if (stricmp(it->value.c_str(), "gzip") == 0 ||
            stricmp(it->value.c_str(), "x-gzip") == 0) {
            stream.reset(new GzipStream(stream));
        } else if (stricmp(it->value.c_str(), "compress") == 0 ||
            stricmp(it->value.c_str(), "x-compress") == 0) {
            MORDOR_ASSERT(false);
        } else if (stricmp(it->value.c_str(), "identity") == 0) {
            MORDOR_ASSERT(false);
        } else {
            MORDOR_ASSERT(false);
        }
    }
    if (stream != baseStream) {
    } else if (entity.contentLength != ~0ull) {
        stream.reset(new LimitedStream(stream, entity.contentLength));
    } else if (entity.contentType.type == "multipart") {
        // Getting stream to pass to multipart; self-delimiting
    } else {
        // Delimited by closing the connection
    }
    NotifyStream::ptr notify(new NotifyStream(stream));
    stream = notify;
    notify->notifyOnClose = notifyOnEof;
    notify->notifyOnEof = notifyOnEof;
    notify->notifyOnException = notifyOnException;
    return stream;
}

}}