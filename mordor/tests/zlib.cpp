#include "mordor/pch.h"

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "mordor/exception.h"

#include "mordor/streams/deflate.h"
#include "mordor/streams/gzip.h"
#include "mordor/streams/lzma2.h"
#include "mordor/streams/memory.h"
#include "mordor/streams/random.h"
#include "mordor/streams/singleplex.h"
#include "mordor/streams/zlib.h"
#include "mordor/test/test.h"

using namespace Mordor;
using namespace Mordor::Test;

static const unsigned char test_uncompressed[] = {
0x55, 0x6e, 0x66, 0x6f, 0x72, 0x74, 0x75, 0x6e, 0x61, 0x74,
0x65, 0x6c, 0x79, 0x2c, 0x20, 0x63, 0x6f, 0x6d, 0x70, 0x75,
0x74, 0x65, 0x72, 0x73, 0x20, 0x61, 0x72, 0x65, 0x20, 0x76,
0x75, 0x6c, 0x6e, 0x65, 0x72, 0x61, 0x62, 0x6c, 0x65, 0x20,
0x74, 0x6f, 0x20, 0x68, 0x61, 0x72, 0x64, 0x20, 0x64, 0x72,
0x69, 0x76, 0x65, 0x20, 0x63, 0x72, 0x61, 0x73, 0x68, 0x65,
0x73, 0x2c, 0x20, 0x76, 0x69, 0x72, 0x75, 0x73, 0x20, 0x61,
0x74, 0x74, 0x61, 0x63, 0x6b, 0x73, 0x2c, 0x20, 0x74, 0x68,
0x65, 0x66, 0x74, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6e, 0x61,
0x74, 0x75, 0x72, 0x61, 0x6c, 0x20, 0x64, 0x69, 0x73, 0x61,
0x73, 0x74, 0x65, 0x72, 0x73, 0x2c, 0x20, 0x77, 0x68, 0x69,
0x63, 0x68, 0x20, 0x63, 0x61, 0x6e, 0x20, 0x65, 0x72, 0x61,
0x73, 0x65, 0x20, 0x65, 0x76, 0x65, 0x72, 0x79, 0x74, 0x68,
0x69, 0x6e, 0x67, 0x20, 0x69, 0x6e, 0x20, 0x61, 0x6e, 0x20,
0x69, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x2e, 0x0d, 0x0a
};

static const unsigned char test_deflate[] = {
0x15, 0xcc, 0xc1, 0x0d, 0x42, 0x31, 0x0c, 0x03, 0xd0, 0x3b,
0x12, 0x3b, 0x78, 0x80, 0x8a, 0x6d, 0x18, 0x20, 0xb4, 0xf9,
0xa4, 0xa2, 0xa4, 0x28, 0x49, 0x8b, 0xfe, 0xf6, 0x84, 0x8b,
0x25, 0x4b, 0xf6, 0xbb, 0xeb, 0x31, 0x2d, 0x96, 0x52, 0xf0,
0x38, 0x0b, 0xea, 0x7c, 0x7f, 0x56, 0xb0, 0x39, 0xc8, 0x18,
0x7b, 0x0d, 0x65, 0xa3, 0xc7, 0x60, 0xc4, 0x84, 0x90, 0x35,
0x34, 0xeb, 0x9b, 0x51, 0x8d, 0x5c, 0xd8, 0x0b, 0x76, 0xb7,
0x95, 0xdb, 0x08, 0xaa, 0xaf, 0xac, 0x21, 0x7c, 0x04, 0x48,
0x1b, 0x12, 0x5c, 0x46, 0x03, 0xad, 0x3b, 0xf9, 0x1f, 0x2c,
0xf8, 0x4a, 0xaf, 0x82, 0x4a, 0x8a, 0x34, 0x9d, 0xc1, 0x9b,
0xed, 0x0c, 0xe9, 0xfa, 0x44, 0xd7, 0x3c, 0x65, 0x7a, 0x90,
0xc6, 0xed, 0x7a, 0xf9, 0x01
};

static const unsigned char test_gzip[] = {
0x1f, 0x8b, 0x08, 0x08, 0x4f, 0xa7, 0xd4, 0x4a, 0x00, 0x03,
0x73, 0x6f, 0x6d, 0x65, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x00,
0x15, 0xcc, 0xc1, 0x0d, 0x42, 0x31, 0x0c, 0x03, 0xd0, 0x3b,
0x12, 0x3b, 0x78, 0x80, 0x8a, 0x6d, 0x18, 0x20, 0xb4, 0xf9,
0xa4, 0xa2, 0xa4, 0x28, 0x49, 0x8b, 0xfe, 0xf6, 0x84, 0x8b,
0x25, 0x4b, 0xf6, 0xbb, 0xeb, 0x31, 0x2d, 0x96, 0x52, 0xf0,
0x38, 0x0b, 0xea, 0x7c, 0x7f, 0x56, 0xb0, 0x39, 0xc8, 0x18,
0x7b, 0x0d, 0x65, 0xa3, 0xc7, 0x60, 0xc4, 0x84, 0x90, 0x35,
0x34, 0xeb, 0x9b, 0x51, 0x8d, 0x5c, 0xd8, 0x0b, 0x76, 0xb7,
0x95, 0xdb, 0x08, 0xaa, 0xaf, 0xac, 0x21, 0x7c, 0x04, 0x48,
0x1b, 0x12, 0x5c, 0x46, 0x03, 0xad, 0x3b, 0xf9, 0x1f, 0x2c,
0xf8, 0x4a, 0xaf, 0x82, 0x4a, 0x8a, 0x34, 0x9d, 0xc1, 0x9b,
0xed, 0x0c, 0xe9, 0xfa, 0x44, 0xd7, 0x3c, 0x65, 0x7a, 0x90,
0xc6, 0xed, 0x7a, 0xf9, 0x01, 0x32, 0x81, 0xa0, 0xbb, 0x96,
0x00, 0x00, 0x00
};

static const unsigned char test_zlib[] = {
0x78, 0x9c, 0x15, 0xcc, 0xc1, 0x0d, 0x42, 0x31, 0x0c, 0x03,
0xd0, 0x3b, 0x12, 0x3b, 0x78, 0x80, 0x8a, 0x6d, 0x18, 0x20,
0xb4, 0xf9, 0xa4, 0xa2, 0xa4, 0x28, 0x49, 0x8b, 0xfe, 0xf6,
0x84, 0x8b, 0x25, 0x4b, 0xf6, 0xbb, 0xeb, 0x31, 0x2d, 0x96,
0x52, 0xf0, 0x38, 0x0b, 0xea, 0x7c, 0x7f, 0x56, 0xb0, 0x39,
0xc8, 0x18, 0x7b, 0x0d, 0x65, 0xa3, 0xc7, 0x60, 0xc4, 0x84,
0x90, 0x35, 0x34, 0xeb, 0x9b, 0x51, 0x8d, 0x5c, 0xd8, 0x0b,
0x76, 0xb7, 0x95, 0xdb, 0x08, 0xaa, 0xaf, 0xac, 0x21, 0x7c,
0x04, 0x48, 0x1b, 0x12, 0x5c, 0x46, 0x03, 0xad, 0x3b, 0xf9,
0x1f, 0x2c, 0xf8, 0x4a, 0xaf, 0x82, 0x4a, 0x8a, 0x34, 0x9d,
0xc1, 0x9b, 0xed, 0x0c, 0xe9, 0xfa, 0x44, 0xd7, 0x3c, 0x65,
0x7a, 0x90, 0xc6, 0xed, 0x7a, 0xf9, 0x01, 0xaa, 0x68, 0x37,
0x41
};

static const unsigned char test_lzma[] = {
    0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00, 0x00, 0x04, 0xe6, 0xd6,
    0xb4, 0x46, 0x02, 0x00, 0x21, 0x01, 0x16, 0x00, 0x00, 0x00,
    0x74, 0x2f, 0xe5, 0xa3, 0xe0, 0x00, 0x95, 0x00, 0x81, 0x5d,
    0x00, 0x2a, 0x9b, 0x88, 0xc7, 0x22, 0xba, 0xb9, 0xc7, 0xda,
    0x2e, 0xfd, 0x50, 0x04, 0x6f, 0xa1, 0xf1, 0xef, 0x31, 0xa5,
    0xa3, 0x9c, 0x1d, 0xee, 0x4e, 0xc9, 0xc3, 0x28, 0x41, 0x83,
    0x3f, 0xab, 0x84, 0x5c, 0xb6, 0x2d, 0x69, 0x16, 0xf8, 0xe8,
    0xf6, 0xd3, 0xdb, 0x42, 0x9b, 0xab, 0x22, 0x66, 0x76, 0xf4,
    0x30, 0xec, 0xc2, 0x26, 0x1a, 0x52, 0xb8, 0x07, 0x60, 0x63,
    0xc4, 0x51, 0xad, 0x0d, 0xb0, 0xbe, 0x2b, 0x67, 0xab, 0x12,
    0xab, 0xaa, 0x51, 0x03, 0xfa, 0x8e, 0xf6, 0x65, 0x32, 0xc5,
    0x39, 0x12, 0x3d, 0x04, 0xf1, 0x59, 0x95, 0x9b, 0xd4, 0xe3,
    0x47, 0x23, 0x0b, 0xd9, 0xfb, 0x89, 0xe3, 0x2b, 0x5f, 0x98,
    0xe6, 0xc2, 0x4a, 0x92, 0x67, 0xbc, 0x2e, 0x57, 0x6a, 0xfb,
    0x59, 0x72, 0x1d, 0x07, 0xba, 0xa1, 0x00, 0x0e, 0x8e, 0xf4,
    0x44, 0x6d, 0xba, 0xb7, 0x55, 0xab, 0xd3, 0xaa, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xb2, 0x9c, 0x38, 0x19, 0xed, 0x3d,
    0xcd, 0x85, 0x00, 0x01, 0x9d, 0x01, 0x96, 0x01, 0x00, 0x00,
    0xbf, 0x2c, 0x24, 0xc6, 0xb1, 0xc4, 0x67, 0xfb, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x59, 0x5a
};

// for decompression, we will take the pre-compressed test data for the compression method,
// decompress it, and compare it to the original data
template <class StreamType>
void testDecompress(const unsigned char *test_data, size_t size)
{
    Buffer controlBuf;
    controlBuf.copyIn(test_uncompressed, sizeof(test_uncompressed));

    Buffer compressed;
    compressed.copyIn(test_data, size);
    Stream::ptr memstream(new MemoryStream(compressed));
    Stream::ptr readplex(new SingleplexStream(memstream, SingleplexStream::READ));
    StreamType teststream(readplex);
    Buffer testBuf;
    while(0 < teststream.read(testBuf, 4096));

    MORDOR_TEST_ASSERT( controlBuf == testBuf );
}

// for compression, we will compress some data, decompress it, and compare to the original.
// we can't just compare to the existing compressed data test, because there is more than one
// valid compressed representation of a file--but we verified decompression works above.
template <class StreamType>
void testCompress()
{
    Buffer origData;
    origData.copyIn(test_uncompressed, sizeof(test_uncompressed));

    // compress from origData to compressed
    boost::shared_ptr<MemoryStream> memstream(new MemoryStream());
    Stream::ptr writeplex(new SingleplexStream(memstream, SingleplexStream::WRITE));
    StreamType teststream(writeplex);
    teststream.write(origData, origData.readAvailable());
    teststream.close();

    // decompress from compressed to decomp
    Buffer decomp;
    Stream::ptr memstream2(new MemoryStream(memstream->buffer()));
    Stream::ptr readplex(new SingleplexStream(memstream2, SingleplexStream::READ));
    StreamType teststream2(readplex);
    while(0 < teststream2.read(decomp, 4096));

    // compare origData to decomp
    MORDOR_TEST_ASSERT( origData == decomp );
}

// for decompression in inverse mode of operation, we will take the pre-compressed test data
// for the compression method, decompress it, and compare it to the original data.
template <class StreamType>
void testDecompressInverse(const unsigned char *test_data, size_t data_size)
{
    Buffer compressed;
    compressed.copyIn(test_data, data_size);
    Buffer controlBuf;
    controlBuf.copyIn(test_uncompressed, sizeof(test_uncompressed));

    // decompress from compressed to decomp
    boost::shared_ptr<MemoryStream> memstream(new MemoryStream());
    Stream::ptr writeplex(new SingleplexStream(memstream, SingleplexStream::WRITE));
    StreamType teststream(writeplex, true, true);
    teststream.write(compressed, 4096);
    teststream.close();

    Buffer decomp = memstream->buffer();
    MORDOR_TEST_ASSERT( controlBuf == decomp );
}

// for compression in inverse mode of operation, we will compress some data, decompress it,
// and compare to the original.
template <class StreamType>
void testCompressInverse(size_t buffer_size)
{
    Buffer uncompressed;
    RandomStream rand;
    rand.read(uncompressed, buffer_size);

    //compress on read
    Stream::ptr memstream(new MemoryStream(uncompressed));
    Stream::ptr readplex(new SingleplexStream(memstream, SingleplexStream::READ));
    StreamType teststream(readplex, true, true);
    Buffer testBuf;
    while(0 < teststream.read(testBuf, buffer_size));
    teststream.close();

    // decompress from compressed to decomp
    Buffer decomp;
    Stream::ptr memstream2(new MemoryStream(testBuf));
    Stream::ptr readplex2(new SingleplexStream(memstream2, SingleplexStream::READ));
    StreamType teststream2(readplex2);
    while(0 < teststream2.read(decomp, buffer_size));

    // compare decomp to original Data
    MORDOR_TEST_ASSERT( uncompressed == decomp );
}

MORDOR_UNITTEST(ZlibStream, compress)
{
    testCompress<ZlibStream>();
    //big buffer
    testCompressInverse<ZlibStream>(4096);
    //set small buffer, multiple read call test in ZlibStream Inverse Mode.
    testCompressInverse<ZlibStream>(50);
    //larger than max buffer size(65536)
    testCompressInverse<ZlibStream>(409600);
}

MORDOR_UNITTEST(ZlibStream, decompress)
{
    testDecompress<ZlibStream>(test_zlib, sizeof(test_zlib));
    testDecompressInverse<ZlibStream>(test_zlib, sizeof(test_zlib));
}

MORDOR_UNITTEST(GzipStream, compress)
{
    testCompress<GzipStream>();
    testCompressInverse<GzipStream>(4096);
    testCompressInverse<GzipStream>(50);
}

MORDOR_UNITTEST(GzipStream, decompress)
{
    testDecompress<GzipStream>(test_gzip, sizeof(test_gzip));
    testDecompressInverse<GzipStream>(test_gzip, sizeof(test_gzip));
}

MORDOR_UNITTEST(DeflateStream, compress)
{
    testCompress<DeflateStream>();
    testCompressInverse<DeflateStream>(4096);
    testCompressInverse<DeflateStream>(50);
}

MORDOR_UNITTEST(DeflateStream, decompress)
{
    testDecompress<DeflateStream>(test_deflate, sizeof(test_deflate));
    testDecompressInverse<DeflateStream>(test_deflate, sizeof(test_deflate));
}

#if !defined(HAVE_CONFIG_H) || defined(HAVE_LIBLZMA)

// enable lzma by default

MORDOR_UNITTEST(LZMAStream, compress)
{
    testCompress<LZMAStream>();
}

MORDOR_UNITTEST(LZMAStream, decompress)
{
    testDecompress<LZMAStream>(test_lzma, sizeof(test_lzma));
}

MORDOR_UNITTEST(LZMAStream, badFormat)
{
    // feed a zlib stream to LZMAStream, and see what will happen.
    MORDOR_TEST_ASSERT_EXCEPTION(testDecompress<LZMAStream>(test_zlib, sizeof(test_lzma)),
                                 UnknownLZMAFormatException);
}

MORDOR_UNITTEST(LZMAStream, corruptedStream)
{
    // fake a LZMA2 stream with correct header, but corrupted body
    static const size_t HEADER_SIZE = (6 /* the magic */ +
                                       1 /* stream flags */ +
                                       32 / 8 /* the CRC32 */);
    static const size_t STREAM_SIZE = sizeof(test_lzma);
    unsigned char corrupted[STREAM_SIZE];
    memcpy(corrupted, test_lzma, STREAM_SIZE);
    // make sure we don't write out of bound.
    MORDOR_ASSERT(HEADER_SIZE + 42 + 42 < STREAM_SIZE);
    // so we keep the header unchanged, but just mess up with the stream body.
    memset(corrupted + HEADER_SIZE + 42, 42, 42);
    MORDOR_TEST_ASSERT_EXCEPTION(testDecompress<LZMAStream>(test_zlib, STREAM_SIZE),
                                 UnknownLZMAFormatException);
}

#endif
