// kernel/types.h

#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#define NULL ((void *)0)

#ifdef __LP64__
typedef signed long int64_t;
typedef unsigned long uint64_t;
#endif

typedef unsigned int size_t;
typedef signed int ssize_t;

typedef unsigned long uintptr_t;
typedef signed long intptr_t;

#define Bool int
#define true 1
#define false 0

#endif
