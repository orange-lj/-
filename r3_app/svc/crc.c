#include "crc.h"

ULONG CRC_Adler32(const UCHAR* data, int len)
{
    const ULONG MOD_ADLER = 65521;
    ULONG a = 1, b = 0;

    while (len) {
        size_t tlen = len > 5550 ? 5550 : len;
        len -= tlen;
        do {
            a += *data++;
            b += a;
        } while (--tlen);
        a = (a & 0xffff) + (a >> 16) * (65536 - MOD_ADLER);
        b = (b & 0xffff) + (b >> 16) * (65536 - MOD_ADLER);
    }

    if (a >= MOD_ADLER)
        a -= MOD_ADLER;
    b = (b & 0xffff) + (b >> 16) * (65536 - MOD_ADLER);
    if (b >= MOD_ADLER)
        b -= MOD_ADLER;

    b = (b << 16) | a;
    return b;
}
