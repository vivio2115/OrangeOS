
#ifndef KERNEL_H
#define KERNEL_H


typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

typedef unsigned long size_t;


#ifdef __cplusplus
    #define NULL nullptr
#else
    #define NULL ((void*)0)
#endif


#define ORANGEOS_VERSION "0.1.1"
#define ORANGEOS_NAME "OrangeOS"


#define KERNEL_START 0x10000
#define KERNEL_STACK 0x90000
#define HEAP_START 0x200000  
#define HEAP_SIZE 0x400000   


extern "C" void kernel_main();

#endif 
