#include "string.h"

void *
memcpy(void *dest, const void *src, ulong n)
{
    char *d = dest;
    const char *s = src;

    while (n--)
        *d++ = *s++;

    return dest;
}

void *
memset(void *dst, int c, ulong n)
{
    uchar *p = (uchar *)dst;
    for (uint i = 0; i < n; i++)
        p[i] = (uchar)c;

    return dst;
}

ulong
strlen(const char *s)
{
    ulong len = 0;
    while (*s++)
        len++;

    return len;
}

ulong
strnlen(const char *s, ulong maxlen)
{
    ulong len = 0;
    while (len < maxlen && s[len] != '\0')
        len++;

    return len;
}
