/**
 * @file utils.h
 * @brief utility header.
 * @author mopp
 * @version 0.1
 * @date 2014-09-27
 */


#ifndef _UTILS_H_
#define _UTILS_H_



#include <stddef.h>


extern void *memchr(const void *, int, size_t);
extern int memcmp(const void *, const void *, size_t);
extern void* memcpy(void* restrict, const void* restrict, size_t);
extern void *memset(void *, int, size_t);
extern int strcmp(const char *, const char *);
extern char *strcpy(char *, const char *);
extern size_t strlen(const char *);
extern int isdigit(int);

extern int printf(const char*, ...);
extern char* itoa(int, char*, int);
extern int puts(const char *);
extern int putchar(int);



#endif
