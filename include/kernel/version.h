#ifndef VERSION_H
#define VERSION_H

#define OS_NAME "OrangeOS"
#define OS_VERSION "1.1"

#include "version_build.h"

#define OS_VERSION_STRING OS_NAME " v" OS_VERSION " (" OS_BUILD_HASH ")"
#define OS_FULL_INFO "Version: " OS_VERSION " (" OS_BUILD_HASH ") - " OS_BUILD_DATE " " OS_BUILD_TIME

#endif 
