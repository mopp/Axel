/************************************************************
 * File: include/String.h
 * Description: String Header.
 ************************************************************/

#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>


extern void *memchr(const void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);
extern void *memcpy(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
extern int strcmp(const char *, const char *);
extern char *strcpy(char *, const char *);
extern size_t strlen(const char *);



#endif
