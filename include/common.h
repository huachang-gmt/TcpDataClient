/******************************************************************************
 * @file    common.h
 * @brief   Common definitions for TcpDataClient
 *
 * Copyright (c) 2026
 *
 * This file is shared by all platforms.
 * It must remain platform-independent.
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_COMMON_H
#define TCP_DATA_CLIENT_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Standard Headers
 *============================================================================*/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*=============================================================================
 * Version
 *============================================================================*/

#define TCP_DATA_CLIENT_VERSION_MAJOR      (0U)
#define TCP_DATA_CLIENT_VERSION_MINOR      (1U)
#define TCP_DATA_CLIENT_VERSION_PATCH      (0U)

/*=============================================================================
 * Platform Detection
 *============================================================================*/

#if defined(_WIN32)

#define TCP_PLATFORM_WINDOWS    1

#elif defined(__linux__)

#define TCP_PLATFORM_LINUX      1

#elif defined(STM32H743xx)  || \
      defined(STM32H755xx)  || \
      defined(STM32H750xx)

#define TCP_PLATFORM_STM32      1

#else

#error Unsupported platform

#endif

/*=============================================================================
 * Common Macros
 *============================================================================*/

#define TCP_UNUSED(x)           ((void)(x))

#define TCP_ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))

#define TCP_MIN(a,b)            (((a) < (b)) ? (a) : (b))

#define TCP_MAX(a,b)            (((a) > (b)) ? (a) : (b))

/*=============================================================================
 * Structure Packing
 *============================================================================*/

#if defined(_MSC_VER)

#define TCP_PACKED_BEGIN    __pragma(pack(push, 1))
#define TCP_PACKED_END      __pragma(pack(pop))
#define TCP_PACKED

#elif defined(__GNUC__)

#define TCP_PACKED_BEGIN
#define TCP_PACKED_END
#define TCP_PACKED          __attribute__((packed))

#else

#define TCP_PACKED_BEGIN
#define TCP_PACKED_END
#define TCP_PACKED

#endif

/*=============================================================================
 * Return Code
 *============================================================================*/

typedef enum
{
    TCP_RESULT_OK = 0,

    TCP_RESULT_ERROR,

    TCP_RESULT_TIMEOUT,

    TCP_RESULT_DISCONNECTED,

    TCP_RESULT_BUSY,

    TCP_RESULT_INVALID_PARAMETER,

    TCP_RESULT_NO_MEMORY

} tcp_result_t;

/*=============================================================================
 * Connection State
 *============================================================================*/

typedef enum
{
    TCP_STATE_IDLE = 0,

    TCP_STATE_CONNECTING,

    TCP_STATE_CONNECTED,

    TCP_STATE_DISCONNECTED

} tcp_connection_state_t;

/*=============================================================================
 * Static Assert
 *============================================================================*/

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

#define TCP_STATIC_ASSERT(cond, msg) \
    _Static_assert(cond, msg)

#else

#define TCP_STATIC_ASSERT(cond, msg)

#endif

/*=============================================================================
 * Compile-Time Verification
 *============================================================================*/

TCP_STATIC_ASSERT(sizeof(uint8_t)  == 1, "uint8_t size error");
TCP_STATIC_ASSERT(sizeof(uint16_t) == 2, "uint16_t size error");
TCP_STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t size error");
TCP_STATIC_ASSERT(sizeof(uint64_t) == 8, "uint64_t size error");

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_COMMON_H */