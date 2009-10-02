// Copyright (c) 2009 - Decho Corp.

#include "mordor/common/pch.h"

#include <boost/bind.hpp>

#include "mordor/common/http/client.h"
#include "mordor/common/http/parser.h"
#include "mordor/common/http/server.h"
#include "mordor/common/scheduler.h"
#include "mordor/common/streams/duplex.h"
#include "mordor/common/streams/memory.h"
#include "mordor/common/streams/test.h"
#include "mordor/common/streams/transfer.h"
#include "mordor/test/test.h"

// Simplest success case
TEST_WITH_SUITE(HTTP, simpleRequest)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("GET / HTTP/1.0\r\n\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
}

TEST_WITH_SUITE(HTTP, requestWithQuery)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("POST /ab/d/e/wasdkfe/?ohai=1 HTTP/1.1\r\n\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::POST);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/ab/d/e/wasdkfe/?ohai=1"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 1));
}

TEST_WITH_SUITE(HTTP, emptyRequest)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(!parser.complete());
}

TEST_WITH_SUITE(HTTP, garbageRequest)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("#*((@Nflk:J");
    TEST_ASSERT(parser.error());
    TEST_ASSERT(!parser.complete());
}

TEST_WITH_SUITE(HTTP, missingNewlineRequest)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("GET / HTTP/1.0\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(!parser.complete());
    // Even though it's not complete, we should have parsed as much as was there
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
}

TEST_WITH_SUITE(HTTP, requestWithSimpleHeader)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("GET / HTTP/1.0\r\n"
               "Connection: close\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.general.connection.size(), 1u);
    TEST_ASSERT(request.general.connection.find("close")
        != request.general.connection.end());
}

TEST_WITH_SUITE(HTTP, requestWithComplexHeader)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("GET / HTTP/1.0\r\n"
               "Connection:\r\n"
               " keep-alive,  keep-alive\r\n"
               "\t, close\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.general.connection.size(), 2u);
    TEST_ASSERT(request.general.connection.find("close")
        != request.general.connection.end());
    TEST_ASSERT(request.general.connection.find("keep-alive")
        != request.general.connection.end());
}

TEST_WITH_SUITE(HTTP, ifMatchHeader)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);
    HTTP::ETagSet::iterator it;
    std::ostringstream os;

    parser.run("GET / HTTP/1.0\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT(request.request.ifMatch.empty());
    os << request;
    TEST_ASSERT_EQUAL(os.str(),
        "GET / HTTP/1.0\r\n"
        "\r\n");

    request = HTTP::Request();
    parser.run("GET / HTTP/1.0\r\n"
               "If-Match: *\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.request.ifMatch.size(), 1u);
    it = request.request.ifMatch.begin();
    TEST_ASSERT(it->unspecified);
    TEST_ASSERT(!it->weak);
    TEST_ASSERT_EQUAL(it->value, "");
    os.str("");
    os << request;
    TEST_ASSERT_EQUAL(os.str(),
        "GET / HTTP/1.0\r\n"
        "If-Match: *\r\n"
        "\r\n");

    request = HTTP::Request();
    parser.run("GET / HTTP/1.0\r\n"
               "If-Match: \"\", W/\"other\", \"something\"\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.request.ifMatch.size(), 3u);
    it = request.request.ifMatch.begin();
    TEST_ASSERT(!it->unspecified);
    TEST_ASSERT(!it->weak);
    TEST_ASSERT_EQUAL(it->value, "");
    ++it;
    TEST_ASSERT(!it->unspecified);
    TEST_ASSERT(!it->weak);
    TEST_ASSERT_EQUAL(it->value, "something");
    ++it;
    TEST_ASSERT(!it->unspecified);
    TEST_ASSERT(it->weak);
    TEST_ASSERT_EQUAL(it->value, "other");
    os.str("");
    os << request;
    TEST_ASSERT_EQUAL(os.str(),
        "GET / HTTP/1.0\r\n"
        "If-Match: \"\", \"something\", W/\"other\"\r\n"
        "\r\n");

    // * is only allowed once
    request = HTTP::Request();
    parser.run("GET / HTTP/1.0\r\n"
               "If-Match: \"first\", \"second\", *\r\n"
               "\r\n");
    TEST_ASSERT(parser.error());
    TEST_ASSERT(!parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.request.ifMatch.size(), 2u);
    it = request.request.ifMatch.begin();
    TEST_ASSERT(!it->unspecified);
    TEST_ASSERT(!it->weak);
    TEST_ASSERT_EQUAL(it->value, "first");
    ++it;
    TEST_ASSERT(!it->unspecified);
    TEST_ASSERT(!it->weak);
    TEST_ASSERT_EQUAL(it->value, "second");
}

TEST_WITH_SUITE(HTTP, upgradeHeader)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);
    HTTP::ProductList::iterator it;
    std::ostringstream os;

    parser.run("GET / HTTP/1.0\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT(request.general.upgrade.empty());
    os << request;
    TEST_ASSERT_EQUAL(os.str(),
        "GET / HTTP/1.0\r\n"
        "\r\n");

    request = HTTP::Request();
    parser.run("GET / HTTP/1.0\r\n"
               "Upgrade: HTTP/2.0, SHTTP/1.3, IRC/6.9, RTA/x11\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.general.upgrade.size(), 4u);
    it = request.general.upgrade.begin();
    TEST_ASSERT_EQUAL(it->product, "HTTP");
    TEST_ASSERT_EQUAL(it->version, "2.0");
    ++it;
    TEST_ASSERT_EQUAL(it->product, "SHTTP");
    TEST_ASSERT_EQUAL(it->version, "1.3");
    ++it;
    TEST_ASSERT_EQUAL(it->product, "IRC");
    TEST_ASSERT_EQUAL(it->version, "6.9");
    ++it;
    TEST_ASSERT_EQUAL(it->product, "RTA");
    TEST_ASSERT_EQUAL(it->version, "x11");
    ++it;
    os.str("");
    os << request;
    TEST_ASSERT_EQUAL(os.str(),
        "GET / HTTP/1.0\r\n"
        "Upgrade: HTTP/2.0, SHTTP/1.3, IRC/6.9, RTA/x11\r\n"
        "\r\n");

    request = HTTP::Request();
    parser.run("GET / HTTP/1.0\r\n"
               "Upgrade: HTTP/2.0, SHTTP/1.<3, IRC/6.9, RTA/x11\r\n"
               "\r\n");
    TEST_ASSERT(parser.error());
    TEST_ASSERT(!parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(request.general.upgrade.size(), 1u);
    it = request.general.upgrade.begin();
    TEST_ASSERT_EQUAL(it->product, "HTTP");
    TEST_ASSERT_EQUAL(it->version, "2.0");
}

TEST_WITH_SUITE(HTTP, serverHeader)
{
    HTTP::Response response;
    HTTP::ResponseParser parser(response);
    HTTP::ProductAndCommentList::iterator it;
    std::ostringstream os;

    parser.run("HTTP/1.0 200 OK\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT(response.general.upgrade.empty());
    os << response;
    TEST_ASSERT_EQUAL(os.str(),
        "HTTP/1.0 200 OK\r\n"
        "\r\n");

    response = HTTP::Response();
    parser.run("HTTP/1.0 200 OK\r\n"
               "Server: Apache/2.2.3 (Debian) mod_fastcgi/2.4.2 mod_python/3.2.10 Python/2.4.4 PHP/4.4.4-8+etch6\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT_EQUAL(response.response.server.size(), 6u);
    it = response.response.server.begin();
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).product, "Apache");
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).version, "2.2.3");
    ++it;
    TEST_ASSERT_EQUAL(boost::get<std::string>(*it), "Debian");
    ++it;
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).product, "mod_fastcgi");
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).version, "2.4.2");
    ++it;
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).product, "mod_python");
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).version, "3.2.10");
    ++it;
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).product, "Python");
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).version, "2.4.4");
    ++it;
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).product, "PHP");
    TEST_ASSERT_EQUAL(boost::get<HTTP::Product>(*it).version, "4.4.4-8+etch6");
    ++it;
    os.str("");
    os << response;
    TEST_ASSERT_EQUAL(os.str(),
        "HTTP/1.0 200 OK\r\n"
        "Server: Apache/2.2.3 (Debian) mod_fastcgi/2.4.2 mod_python/3.2.10 Python/2.4.4 PHP/4.4.4-8+etch6\r\n"
        "\r\n");
}

TEST_WITH_SUITE(HTTP, teHeader)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);
    std::ostringstream os;

    parser.run("GET / HTTP/1.0\r\n"
               "TE: deflate, chunked;q=0, x-gzip;q=0.050\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.request.te.size(), 3u);
    HTTP::AcceptList::iterator it = request.request.te.begin();
    TEST_ASSERT_EQUAL(it->value, "deflate");
    TEST_ASSERT_EQUAL(it->qvalue, ~0u);
    ++it;
    TEST_ASSERT_EQUAL(it->value, "chunked");
    TEST_ASSERT_EQUAL(it->qvalue, 0u);
    ++it;
    TEST_ASSERT_EQUAL(it->value, "x-gzip");
    TEST_ASSERT_EQUAL(it->qvalue, 50u);

    os << request;
    TEST_ASSERT_EQUAL(os.str(),
        "GET / HTTP/1.0\r\n"
        "TE: deflate, chunked;q=0, x-gzip;q=0.05\r\n"
        "\r\n");
}

TEST_WITH_SUITE(HTTP, trailer)
{
    HTTP::EntityHeaders trailer;
    HTTP::TrailerParser parser(trailer);

    parser.run("Content-Type: text/plain\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(trailer.contentType.type, "text");
    TEST_ASSERT_EQUAL(trailer.contentType.subtype, "plain");
}

TEST_WITH_SUITE(HTTP, rangeHeader)
{
    HTTP::Request request;
    HTTP::RequestParser parser(request);

    parser.run("GET / HTTP/1.1\r\n"
               "Range: bytes=0-499, 500-999, -500, 9500-, 0-0,-1\r\n"
               " ,500-600\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(request.requestLine.method, HTTP::GET);
    TEST_ASSERT_EQUAL(request.requestLine.uri, URI("/"));
    TEST_ASSERT_EQUAL(request.requestLine.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(request.request.range.size(), 7u);
    HTTP::RangeSet::const_iterator it = request.request.range.begin();
    TEST_ASSERT_EQUAL(it->first, 0u);
    TEST_ASSERT_EQUAL(it->second, 499u);
    ++it;
    TEST_ASSERT_EQUAL(it->first, 500u);
    TEST_ASSERT_EQUAL(it->second, 999u);
    ++it;
    TEST_ASSERT_EQUAL(it->first, ~0ull);
    TEST_ASSERT_EQUAL(it->second, 500u);
    ++it;
    TEST_ASSERT_EQUAL(it->first, 9500u);
    TEST_ASSERT_EQUAL(it->second, ~0ull);
    ++it;
    TEST_ASSERT_EQUAL(it->first, 0u);
    TEST_ASSERT_EQUAL(it->second, 0u);
    ++it;
    TEST_ASSERT_EQUAL(it->first, ~0ull);
    TEST_ASSERT_EQUAL(it->second, 1u);
    ++it;
    TEST_ASSERT_EQUAL(it->first, 500u);
    TEST_ASSERT_EQUAL(it->second, 600u);
}

TEST_WITH_SUITE(HTTP, contentTypeHeader)
{
    HTTP::Response response;
    HTTP::ResponseParser parser(response);

    parser.run("HTTP/1.1 200 OK\r\n"
               "Content-Type: text/plain\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT_EQUAL(response.entity.contentType.type, "text");
    TEST_ASSERT_EQUAL(response.entity.contentType.subtype, "plain");
}

TEST_WITH_SUITE(HTTP, eTagHeader)
{
    HTTP::Response response;
    HTTP::ResponseParser parser(response);
    std::ostringstream os;

    parser.run("HTTP/1.1 200 OK\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT(response.response.eTag.unspecified);
    TEST_ASSERT(!response.response.eTag.weak);
    TEST_ASSERT_EQUAL(response.response.eTag.value, "");
    os << response;
    TEST_ASSERT_EQUAL(os.str(),
        "HTTP/1.1 200 OK\r\n"
        "\r\n");

    response = HTTP::Response();
    parser.run("HTTP/1.1 200 OK\r\n"
               "ETag: \"\"\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT(!response.response.eTag.unspecified);
    TEST_ASSERT(!response.response.eTag.weak);
    TEST_ASSERT_EQUAL(response.response.eTag.value, "");
    os.str("");
    os << response;
    TEST_ASSERT_EQUAL(os.str(),
        "HTTP/1.1 200 OK\r\n"
        "ETag: \"\"\r\n"
        "\r\n");

    response = HTTP::Response();
    parser.run("HTTP/1.1 200 OK\r\n"
               "ETag: W/\"\"\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT(!response.response.eTag.unspecified);
    TEST_ASSERT(response.response.eTag.weak);
    TEST_ASSERT_EQUAL(response.response.eTag.value, "");

    response = HTTP::Response();
    parser.run("HTTP/1.1 200 OK\r\n"
               "ETag: \"sometag\"\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT(!response.response.eTag.unspecified);
    TEST_ASSERT(!response.response.eTag.weak);
    TEST_ASSERT_EQUAL(response.response.eTag.value, "sometag");

    response = HTTP::Response();
    parser.run("HTTP/1.1 200 OK\r\n"
               "ETag: *\r\n"
               "\r\n");
    TEST_ASSERT(parser.error());
    TEST_ASSERT(!parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");

    response = HTTP::Response();
    parser.run("HTTP/1.1 200 OK\r\n"
               "ETag: token\r\n"
               "\r\n");
    TEST_ASSERT(parser.error());
    TEST_ASSERT(!parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
}

TEST_WITH_SUITE(HTTP, locationHeader)
{
    HTTP::Response response;
    HTTP::ResponseParser parser(response);

    parser.run("HTTP/1.1 307 OK\r\n"
               "Location: /partialObjects/"
        "49ZtbkNPlEEi8T+sQLb5mh9zm1DcyaaRoyHUOC9sEfaKIgLh+eKZNUrqR+j3Iybhx321iz"
        "y3J+Mw7gZmIlVcZrP0qHNDxuEQHMUHLqxhXoXcN18+x4XedNLqc8KhnJtHLXndcKMJu5Cg"
        "xp2BI9NXDDDuBmYiVVxms/Soc0PG4RAcxQcurGFehSY0Wf0fG5eWquA0b0hozVjE4xxyAF"
        "TkSU39Hl3XcsUUMO4GZiJVXGaz9KhzQ8bhEBzFBy6sYV6F9718Fox0OiJ3PqBvo2gr352W"
        "vZBqmEeUV1n0CkcClc0w7gZmIlVcZrP0qHNDxuEQHMUHLqxhXoWapmDUfha0WO9SjTUn4F"
        "Jeht8Gjdy6mYpDqvUbB+3OoDDuBmYiVVxms/Soc0PG4RAcxQcurGFehcefjKkVeAR2HShU"
        "2UpBh5g/89ZP9czSJ8qKSKCPGyHWMO4GZiJVXGaz9KhzQ8bhEBzFBy6sYV6FAig0fJADqV"
        "eInu5RU/pgEXJlZ1MBce/F+rv7MI3g5jgw7gZmIlVcZrP0qHNDxuEQHMUHLqxhXoW4GIxe"
        "C1lnhkTtrAv3jhk17r3ZwL8Fq7CvpUHeAQl/JTDuBmYiVVxms/Soc0PG4RAcxQcurGFehc"
        "s4fMw9uBwTihHQAPFbcyDTjZtTMGlaovGaP6xe1H1TMO4GZiJVXGaz9KhzQ8bhEBzFBy6s"
        "YV6FFAhiH0dzP8E0IRZP+oxeL2JkfxiO5v8r7eWnYtMY8d4w7gZmIlVcZrP0qHNDxuEQHM"
        "UHLqxhXoUgoQ1pQreM2tYMR9QaJ7CsSOSJs+Qi5KIzV50DBUYLDjDuBmYiVVxms/Soc0PG"
        "4RAcxQcurGFehdeUg8nHldHqihIknc3OP/QRtBawAyEFY4p0RKlRxnA0MO4GZiJVXGaz9K"
        "hzQ8bhEBzFBy6sYV6FbRY5v48No3N72yRSA9JiYPhS/YTYcUFz\r\n"
               "\r\n");
    TEST_ASSERT(!parser.error());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::TEMPORARY_REDIRECT);
    TEST_ASSERT_EQUAL(response.status.reason, "OK");
    TEST_ASSERT_EQUAL(response.response.location, "/partialObjects/"
        "49ZtbkNPlEEi8T+sQLb5mh9zm1DcyaaRoyHUOC9sEfaKIgLh+eKZNUrqR+j3Iybhx321iz"
        "y3J+Mw7gZmIlVcZrP0qHNDxuEQHMUHLqxhXoXcN18+x4XedNLqc8KhnJtHLXndcKMJu5Cg"
        "xp2BI9NXDDDuBmYiVVxms/Soc0PG4RAcxQcurGFehSY0Wf0fG5eWquA0b0hozVjE4xxyAF"
        "TkSU39Hl3XcsUUMO4GZiJVXGaz9KhzQ8bhEBzFBy6sYV6F9718Fox0OiJ3PqBvo2gr352W"
        "vZBqmEeUV1n0CkcClc0w7gZmIlVcZrP0qHNDxuEQHMUHLqxhXoWapmDUfha0WO9SjTUn4F"
        "Jeht8Gjdy6mYpDqvUbB+3OoDDuBmYiVVxms/Soc0PG4RAcxQcurGFehcefjKkVeAR2HShU"
        "2UpBh5g/89ZP9czSJ8qKSKCPGyHWMO4GZiJVXGaz9KhzQ8bhEBzFBy6sYV6FAig0fJADqV"
        "eInu5RU/pgEXJlZ1MBce/F+rv7MI3g5jgw7gZmIlVcZrP0qHNDxuEQHMUHLqxhXoW4GIxe"
        "C1lnhkTtrAv3jhk17r3ZwL8Fq7CvpUHeAQl/JTDuBmYiVVxms/Soc0PG4RAcxQcurGFehc"
        "s4fMw9uBwTihHQAPFbcyDTjZtTMGlaovGaP6xe1H1TMO4GZiJVXGaz9KhzQ8bhEBzFBy6s"
        "YV6FFAhiH0dzP8E0IRZP+oxeL2JkfxiO5v8r7eWnYtMY8d4w7gZmIlVcZrP0qHNDxuEQHM"
        "UHLqxhXoUgoQ1pQreM2tYMR9QaJ7CsSOSJs+Qi5KIzV50DBUYLDjDuBmYiVVxms/Soc0PG"
        "4RAcxQcurGFehdeUg8nHldHqihIknc3OP/QRtBawAyEFY4p0RKlRxnA0MO4GZiJVXGaz9K"
        "hzQ8bhEBzFBy6sYV6FbRY5v48No3N72yRSA9JiYPhS/YTYcUFz");
}

TEST_WITH_SUITE(HTTP, versionComparison)
{
    HTTP::Version ver10(1, 0), ver11(1, 1);
    TEST_ASSERT(ver10 == ver10);
    TEST_ASSERT(ver11 == ver11);
    TEST_ASSERT(ver10 != ver11);
    TEST_ASSERT(ver10 <= ver11);
    TEST_ASSERT(ver10 < ver11);
    TEST_ASSERT(ver11 >= ver10);
    TEST_ASSERT(ver11 > ver10);
    TEST_ASSERT(ver10 <= ver10);
    TEST_ASSERT(ver10 >= ver10);
}

static void testQuotingRoundTrip(const std::string &unquoted, const std::string &quoted)
{
    TEST_ASSERT_EQUAL(HTTP::quote(unquoted), quoted);
    TEST_ASSERT_EQUAL(HTTP::unquote(quoted), unquoted);
}

TEST_WITH_SUITE(HTTP, quoting)
{
    // Empty string needs to be quoted (otherwise ambiguous)
    testQuotingRoundTrip("", "\"\"");
    // Tokens do not need to be quoted
    testQuotingRoundTrip("token", "token");
    testQuotingRoundTrip("tom", "tom");
    testQuotingRoundTrip("token.non-separator+chars", "token.non-separator+chars");
    // Whitespace is quoted, but not escaped
    testQuotingRoundTrip("multiple words", "\"multiple words\"");
    testQuotingRoundTrip("\tlotsa  whitespace\t",
        "\"\tlotsa  whitespace\t\"");
    // Backslashes and quotes are escaped
    testQuotingRoundTrip("\"", "\"\\\"\"");
    testQuotingRoundTrip("\\", "\"\\\\\"");
    testQuotingRoundTrip("co\\dy", "\"co\\\\dy\"");
    testQuotingRoundTrip("multiple\\ escape\" sequences\\  ",
        "\"multiple\\\\ escape\\\" sequences\\\\  \"");
    // Separators are quoted, but not escaped
    testQuotingRoundTrip("weird < chars >", "\"weird < chars >\"");
    // CTL gets escaped
    testQuotingRoundTrip(std::string("\0", 1), std::string("\"\\\0\"", 4));
    testQuotingRoundTrip("\x7f", "\"\\\x7f\"");
    // > 127 is quoted, but not escaped
    testQuotingRoundTrip("\x80", "\"\x80\"");

    // ETag even quotes tokens
    TEST_ASSERT_EQUAL(HTTP::quote("token", true), "\"token\"");
}

static void testCommentRoundTrip(const std::string &unquoted, const std::string &quoted)
{
    TEST_ASSERT_EQUAL(HTTP::quote(unquoted, true, true), quoted);
    TEST_ASSERT_EQUAL(HTTP::unquote(quoted), unquoted);
}

TEST_WITH_SUITE(HTTP, comments)
{
    // Empty string needs to be quoted (otherwise ambiguous)
    testCommentRoundTrip("", "()");
    // Tokens are quoted
    testCommentRoundTrip("token", "(token)");
    testCommentRoundTrip("token.non-separator+chars", "(token.non-separator+chars)");
    // Whitespace is quoted, but not escaped
    testCommentRoundTrip("multiple words", "(multiple words)");
    testCommentRoundTrip("\tlotsa  whitespace\t",
        "(\tlotsa  whitespace\t)");
    // Backslashes are escaped
    testCommentRoundTrip("\\", "(\\\\)");
    testCommentRoundTrip("co\\dy", "(co\\\\dy)");
    // Quotes are not escaped
    testCommentRoundTrip("\"", "(\")");
    // Separators are quoted, but not escaped
    testCommentRoundTrip("weird < chars >", "(weird < chars >)");
    // CTL gets escaped
    testCommentRoundTrip(std::string("\0", 1), std::string("(\\\0)", 4));
    testCommentRoundTrip("\x7f", "(\\\x7f)");
    // > 127 is quoted, but not escaped
    testCommentRoundTrip("\x80", "(\x80)");
    // Parens are not escaped, if they're matched
    testCommentRoundTrip("()", "(())");
    testCommentRoundTrip("(", "(\\()");
    testCommentRoundTrip(")", "(\\))");
    testCommentRoundTrip("(()", "((\\())");
    testCommentRoundTrip("())", "(()\\))");
    testCommentRoundTrip(")(", "(\\)\\()");
    testCommentRoundTrip("(()))()", "((())\\)())");
}

static void
httpRequest(HTTP::ServerRequest::ptr request)
{
    switch (request->request().requestLine.method) {
        case HTTP::GET:
        case HTTP::HEAD:
        case HTTP::PUT:
        case HTTP::POST:
            request->response().entity.contentLength = request->request().entity.contentLength;
            request->response().entity.contentType = request->request().entity.contentType;
            request->response().general.transferEncoding = request->request().general.transferEncoding;
            request->response().status.status = HTTP::OK;
            request->response().entity.extension = request->request().entity.extension;
            if (request->hasRequestBody()) {
                if (request->request().requestLine.method != HTTP::HEAD) {
                    if (request->request().entity.contentType.type == "multipart") {
                        Multipart::ptr requestMultipart = request->requestMultipart();
                        Multipart::ptr responseMultipart = request->responseMultipart();
                        for (BodyPart::ptr requestPart = requestMultipart->nextPart();
                            requestPart;
                            requestPart = requestMultipart->nextPart()) {
                            BodyPart::ptr responsePart = responseMultipart->nextPart();
                            responsePart->headers() = requestPart->headers();
                            transferStream(requestPart->stream(), responsePart->stream());
                            responsePart->stream()->close();
                        }
                        responseMultipart->finish();
                    } else {
                        respondStream(request, request->requestStream());
                        return;
                    }
                } else {
                    request->finish();
                }
            } else {
                request->response().entity.contentLength = 0;
                request->finish();
            }
            break;
        default:
            respondError(request, HTTP::METHOD_NOT_ALLOWED);
            break;
    }
}

static void
doSingleRequest(const char *request, HTTP::Response &response)
{
    Stream::ptr input(new MemoryStream(Buffer(request)));
    MemoryStream::ptr output(new MemoryStream());
    Stream::ptr stream(new DuplexStream(input, output));
    HTTP::ServerConnection::ptr conn(new HTTP::ServerConnection(stream, &httpRequest));
    Fiber::ptr mainfiber(new Fiber());
    WorkerPool pool;
    pool.schedule(Fiber::ptr(new Fiber(boost::bind(&HTTP::ServerConnection::processRequests, conn))));
    pool.dispatch();
    HTTP::ResponseParser parser(response);
    parser.run(output->buffer());
    TEST_ASSERT(parser.complete());
    TEST_ASSERT(!parser.error());
}

TEST_WITH_SUITE(HTTPServer, badRequest)
{
    HTTP::Response response;
    doSingleRequest("garbage", response);
    TEST_ASSERT_EQUAL(response.status.status, HTTP::BAD_REQUEST);
}

TEST_WITH_SUITE(HTTPServer, close10)
{
    HTTP::Response response;
    doSingleRequest(
        "GET / HTTP/1.0\r\n"
        "\r\n",
        response);
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT(response.general.connection.find("close") != response.general.connection.end());
}

TEST_WITH_SUITE(HTTPServer, keepAlive10)
{
    HTTP::Response response;
    doSingleRequest(
        "GET / HTTP/1.0\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n",
        response);
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 0));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT(response.general.connection.find("Keep-Alive") != response.general.connection.end());
    TEST_ASSERT(response.general.connection.find("close") == response.general.connection.end());
}

TEST_WITH_SUITE(HTTPServer, noHost11)
{
    HTTP::Response response;
    doSingleRequest(
        "GET / HTTP/1.1\r\n"
        "\r\n",
        response);
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::BAD_REQUEST);
}

TEST_WITH_SUITE(HTTPServer, close11)
{
    HTTP::Response response;
    doSingleRequest(
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "Connection: close\r\n"
        "\r\n",
        response);
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT(response.general.connection.find("close") != response.general.connection.end());
}

TEST_WITH_SUITE(HTTPServer, keepAlive11)
{
    HTTP::Response response;
    doSingleRequest(
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n",
        response);
    TEST_ASSERT_EQUAL(response.status.ver, HTTP::Version(1, 1));
    TEST_ASSERT_EQUAL(response.status.status, HTTP::OK);
    TEST_ASSERT(response.general.connection.find("close") == response.general.connection.end());
}

TEST_WITH_SUITE(HTTPClient, emptyRequest)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.general.connection.insert("close");

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    TEST_ASSERT(!request->hasResponseBody());
#ifdef DEBUG
    TEST_ASSERT_ASSERTED(request->responseStream());
#endif

    // No more requests possible, because we used Connection: close
    TEST_ASSERT_EXCEPTION(conn->request(requestHeaders),
        HTTP::ConnectionVoluntarilyClosedException);
}

TEST_WITH_SUITE(HTTPClient, pipelinedSynchronousRequests)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request1 = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), 0);

    requestHeaders.general.connection.insert("close");
    HTTP::ClientRequest::ptr request2 = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n"
        "GET / HTTP/1.1\r\n"
        "Connection: close\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), 0);

    // No more requests possible, even pipelined ones, because we used
    // Connection: close
    TEST_ASSERT_EXCEPTION(conn->request(requestHeaders),
        HTTP::ConnectionVoluntarilyClosedException);

    TEST_ASSERT_EQUAL(request1->response().status.status, HTTP::OK);
    // Can't test for if half of the stream has been consumed here, because it
    // will be in a buffer somewhere
    TEST_ASSERT_EQUAL(request2->response().status.status, HTTP::OK);
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), responseStream->size());
}

#ifdef DEBUG
TEST_WITH_SUITE(HTTPClient, pipelinedSynchronousRequestsAssertion)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "\r\n\r\n"
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request1 = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), 0);

    requestHeaders.general.connection.insert("close");
    HTTP::ClientRequest::ptr request2 = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n"
        "GET / HTTP/1.1\r\n"
        "Connection: close\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), 0);

    // No more requests possible, even pipelined ones, because we used
    // Connection: close
    TEST_ASSERT_EXCEPTION(conn->request(requestHeaders),
        HTTP::ConnectionVoluntarilyClosedException);

    TEST_ASSERT_EQUAL(request1->response().status.status, HTTP::OK);
    // We're in a single fiber, and we haven't finished the previous response,
    // so the scheduler will exit when this tries to block, returning
    // immediately, and triggering an assertion that request2 isn't the current
    // response
    Fiber::ptr mainFiber(new Fiber());
    IOManager ioManager;
    TEST_ASSERT_ASSERTED(request2->response());
}
#endif

TEST_WITH_SUITE(HTTPClient, simpleResponseBody)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "Connection: close\r\n"
        "\r\n"
        "hello")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.general.connection.insert("close");

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(request->hasResponseBody());
    TEST_ASSERT(request->responseStream()->supportsRead());
    TEST_ASSERT(!request->responseStream()->supportsWrite());
    TEST_ASSERT(!request->responseStream()->supportsSeek());
    TEST_ASSERT(request->responseStream()->supportsSize());
    TEST_ASSERT(!request->responseStream()->supportsTruncate());
    TEST_ASSERT(!request->responseStream()->supportsFind());
    TEST_ASSERT(!request->responseStream()->supportsUnread());
    TEST_ASSERT_EQUAL(request->responseStream()->size(), 5);

    // Verify response itself
    MemoryStream responseBody;
    transferStream(request->responseStream(), responseBody);
    TEST_ASSERT(responseBody.buffer() == "hello");
}

TEST_WITH_SUITE(HTTPClient, chunkedResponseBody)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "5\r\n"
        "hello"
        "\r\n"
        "0\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(request->hasResponseBody());
    TEST_ASSERT(request->responseStream()->supportsRead());
    TEST_ASSERT(!request->responseStream()->supportsWrite());
    TEST_ASSERT(!request->responseStream()->supportsSeek());
    TEST_ASSERT(!request->responseStream()->supportsSize());
    TEST_ASSERT(!request->responseStream()->supportsTruncate());
    TEST_ASSERT(!request->responseStream()->supportsFind());
    TEST_ASSERT(!request->responseStream()->supportsUnread());

    // Verify response itself
    MemoryStream responseBody;
    transferStream(request->responseStream(), responseBody);
    TEST_ASSERT(responseBody.buffer() == "hello");
}

TEST_WITH_SUITE(HTTPClient, trailerResponse)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "0\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(request->hasResponseBody());
    TEST_ASSERT(request->responseStream()->supportsRead());
    TEST_ASSERT(!request->responseStream()->supportsWrite());
    TEST_ASSERT(!request->responseStream()->supportsSeek());
    TEST_ASSERT(!request->responseStream()->supportsSize());
    TEST_ASSERT(!request->responseStream()->supportsTruncate());
    TEST_ASSERT(!request->responseStream()->supportsFind());
    TEST_ASSERT(!request->responseStream()->supportsUnread());

    // Verify response itself
    MemoryStream responseBody;
    transferStream(request->responseStream(), responseBody);
    TEST_ASSERT(responseBody.buffer() == "");

    // Trailer!
    TEST_ASSERT_EQUAL(request->responseTrailer().contentType.type, "text");
    TEST_ASSERT_EQUAL(request->responseTrailer().contentType.subtype, "plain");
}

TEST_WITH_SUITE(HTTPClient, simpleRequestBody)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.method = HTTP::PUT;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.general.connection.insert("close");
    requestHeaders.entity.contentLength = 5;

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    // Nothing has been flushed yet
    TEST_ASSERT_EQUAL(requestStream->size(), 0);
    Stream::ptr requestBody = request->requestStream();
    // Verify stream characteristics
    TEST_ASSERT(!requestBody->supportsRead());
    TEST_ASSERT(requestBody->supportsWrite());
    TEST_ASSERT(!requestBody->supportsSeek());
    TEST_ASSERT(requestBody->supportsSize());
    TEST_ASSERT(!requestBody->supportsTruncate());
    TEST_ASSERT(!requestBody->supportsFind());
    TEST_ASSERT(!requestBody->supportsUnread());
    TEST_ASSERT_EQUAL(requestBody->size(), 5);

    // Force a flush (of the headers)
    requestBody->flush();
    TEST_ASSERT(requestStream->buffer() ==
        "PUT / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "Content-Length: 5\r\n"
        "\r\n");

    // Write the body
    TEST_ASSERT_EQUAL(requestBody->write("hello"), 5u);
    requestBody->close();
    TEST_ASSERT_EQUAL(requestBody->size(), 5);

    TEST_ASSERT(requestStream->buffer() ==
        "PUT / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello");

    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(!request->hasResponseBody());
}

TEST_WITH_SUITE(HTTPClient, chunkedRequestBody)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.method = HTTP::PUT;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.general.connection.insert("close");
    requestHeaders.general.transferEncoding.push_back(HTTP::ValueWithParameters("chunked"));

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    // Nothing has been flushed yet
    TEST_ASSERT_EQUAL(requestStream->size(), 0);
    Stream::ptr requestBody = request->requestStream();
    // Verify stream characteristics
    TEST_ASSERT(!requestBody->supportsRead());
    TEST_ASSERT(requestBody->supportsWrite());
    TEST_ASSERT(!requestBody->supportsSeek());
    TEST_ASSERT(!requestBody->supportsSize());
    TEST_ASSERT(!requestBody->supportsTruncate());
    TEST_ASSERT(!requestBody->supportsFind());
    TEST_ASSERT(!requestBody->supportsUnread());

    // Force a flush (of the headers)
    requestBody->flush();
    TEST_ASSERT(requestStream->buffer() ==
        "PUT / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n");

    // Write the body
    TEST_ASSERT_EQUAL(requestBody->write("hello"), 5u);
    TEST_ASSERT_EQUAL(requestBody->write("world"), 5u);
    requestBody->close();

    TEST_ASSERT(requestStream->buffer() ==
        "PUT / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "hello"
        "\r\n"
        "5\r\n"
        "world"
        "\r\n"
        "0\r\n"
        // No trailers
        "\r\n");

    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(!request->hasResponseBody());
}

TEST_WITH_SUITE(HTTPClient, simpleRequestPartialWrites)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    TestStream::ptr testStream(new TestStream(duplexStream));
    testStream->maxReadSize(10);
    testStream->maxWriteSize(10);
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(testStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.method = HTTP::GET;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.general.connection.insert("close");

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);

    // Force a flush (of the headers)
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.0\r\n"
        "Connection: close\r\n"
        "\r\n");

    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(!request->hasResponseBody());
}

static void pipelinedRequests(HTTP::ClientConnection::ptr conn,
    int &sequence)
{
    TEST_ASSERT_EQUAL(++sequence, 2);

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request2 = conn->request(requestHeaders);
    TEST_ASSERT_EQUAL(++sequence, 4);

    TEST_ASSERT_EQUAL(request2->response().status.status, HTTP::NOT_FOUND);
    TEST_ASSERT_EQUAL(++sequence, 6);
}

TEST_WITH_SUITE(HTTPClient, pipelinedRequests)
{
    Fiber::ptr mainFiber(new Fiber());
    WorkerPool pool;
    int sequence = 1;

    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n"
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.method = HTTP::PUT;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";
    requestHeaders.entity.contentLength = 7;

    HTTP::ClientRequest::ptr request1 = conn->request(requestHeaders);
    
    // Start the second request, which will yield to us when it can't use the conn
    Fiber::ptr request2Fiber(new Fiber(boost::bind(&pipelinedRequests,
        conn, boost::ref(sequence))));
    pool.schedule(request2Fiber);
    pool.dispatch();
    TEST_ASSERT_EQUAL(++sequence, 3);

    TEST_ASSERT_EQUAL(request1->requestStream()->write("hello\r\n"), 7u);
    request1->requestStream()->close();

    // Nothing has been sent to the server yet (it's buffered up), because
    // there is a pipelined request waiting, and we only flush after
    // the last request
    TEST_ASSERT_EQUAL(requestStream->size(), 0);

    pool.dispatch();
    TEST_ASSERT_EQUAL(++sequence, 5);
    
    // Both requests have been sent now (flush()es after last request)
    TEST_ASSERT(requestStream->buffer() ==
        "PUT / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        "hello\r\n"
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    
    // Nothing has been read yet
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), 0);

    TEST_ASSERT_EQUAL(request1->response().status.status, HTTP::OK);
    pool.dispatch();
    TEST_ASSERT_EQUAL(++sequence, 7);

    // Both responses have been read now
    TEST_ASSERT_EQUAL(responseStream->seek(0, Stream::CURRENT), responseStream->size());
}

TEST_WITH_SUITE(HTTPClient, missingTrailerResponse)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "0\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(request->hasResponseBody());
    TEST_ASSERT(request->responseStream()->supportsRead());
    TEST_ASSERT(!request->responseStream()->supportsWrite());
    TEST_ASSERT(!request->responseStream()->supportsSeek());
    TEST_ASSERT(!request->responseStream()->supportsSize());
    TEST_ASSERT(!request->responseStream()->supportsTruncate());
    TEST_ASSERT(!request->responseStream()->supportsFind());
    TEST_ASSERT(!request->responseStream()->supportsUnread());

    // Trailer hasn't been read yet
#ifdef DEBUG
    TEST_ASSERT_ASSERTED(request->responseTrailer());
#endif

    // Verify response itself
    MemoryStream responseBody;
    transferStream(request->responseStream(), responseBody);
    TEST_ASSERT(responseBody.buffer() == "");

    // Missing trailer
    TEST_ASSERT_EXCEPTION(request->responseTrailer(), HTTP::IncompleteMessageHeaderException);
}

TEST_WITH_SUITE(HTTPClient, badTrailerResponse)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n"
        "\r\n"
        "0\r\n"
        "Content-Type: garbage\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(request->hasResponseBody());
    TEST_ASSERT(request->responseStream()->supportsRead());
    TEST_ASSERT(!request->responseStream()->supportsWrite());
    TEST_ASSERT(!request->responseStream()->supportsSeek());
    TEST_ASSERT(!request->responseStream()->supportsSize());
    TEST_ASSERT(!request->responseStream()->supportsTruncate());
    TEST_ASSERT(!request->responseStream()->supportsFind());
    TEST_ASSERT(!request->responseStream()->supportsUnread());

    // Trailer hasn't been read yet
#ifdef DEBUG
    TEST_ASSERT_ASSERTED(request->responseTrailer());
#endif

    // Verify response itself
    MemoryStream responseBody;
    transferStream(request->responseStream(), responseBody);
    TEST_ASSERT(responseBody.buffer() == "");

    // Missing trailer
    TEST_ASSERT_EXCEPTION(request->responseTrailer(), HTTP::BadMessageHeaderException);
}

TEST_WITH_SUITE(HTTPClient, cancelRequestSingle)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.method = HTTP::PUT;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";
    requestHeaders.entity.contentLength = 5;

    HTTP::ClientRequest::ptr request = conn->request(requestHeaders);
    // Nothing has been flushed yet
    TEST_ASSERT_EQUAL(requestStream->size(), 0);
    request->cancel();
    TEST_ASSERT_EQUAL(requestStream->size(), 0);

    TEST_ASSERT_EXCEPTION(conn->request(requestHeaders), HTTP::PriorRequestFailedException);
}

TEST_WITH_SUITE(HTTPClient, cancelResponseSingle)
{
    MemoryStream::ptr requestStream(new MemoryStream());
    MemoryStream::ptr responseStream(new MemoryStream(Buffer(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "hello")));
    DuplexStream::ptr duplexStream(new DuplexStream(responseStream, requestStream));
    HTTP::ClientConnection::ptr conn(new HTTP::ClientConnection(duplexStream));

    HTTP::Request requestHeaders;
    requestHeaders.requestLine.uri = "/";
    requestHeaders.request.host = "garbage";

    HTTP::ClientRequest::ptr request1 = conn->request(requestHeaders);
    TEST_ASSERT(requestStream->buffer() ==
        "GET / HTTP/1.1\r\n"
        "Host: garbage\r\n"
        "\r\n");
    TEST_ASSERT_EQUAL(request1->response().status.status, HTTP::OK);
    // Verify response characteristics
    TEST_ASSERT(request1->hasResponseBody());

    HTTP::ClientRequest::ptr request2 = conn->request(requestHeaders);

    request1->cancel(true);

    TEST_ASSERT_EXCEPTION(request2->ensureResponse(), HTTP::PriorRequestFailedException);

    // Verify response can't be read (exception; when using a real socket it might let us
    // read to EOF)
    MemoryStream responseBody;
    TEST_ASSERT_EXCEPTION(transferStream(request1->responseStream(), responseBody),
        BrokenPipeException);

    TEST_ASSERT_EXCEPTION(conn->request(requestHeaders), HTTP::PriorRequestFailedException);
}
