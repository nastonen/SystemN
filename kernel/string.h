#pragma once

#include "types.h"

#define MAXLEN 64

void *memcpy(void *dest, const void *src, ulong n);
void *memset(void *dst, int c, ulong n);
ulong strlen(const char *s);
ulong strnlen(const char *s, ulong maxlen);
