/******************************************************************************
 * @file    statistics.c
 * @brief   Transmission statistics
 ******************************************************************************/

#include "statistics.h"

#include <string.h>

#if TCP_PLATFORM_WINDOWS

#include <windows.h>

static uint64_t get_tick_ms(void)
{
    return (uint64_t)GetTickCount64();
}

#elif TCP_PLATFORM_LINUX

#include <time.h>

static uint64_t get_tick_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((uint64_t)ts.tv_sec * 1000ULL) +
           ((uint64_t)ts.tv_nsec / 1000000ULL);
}

#else

extern uint32_t HAL_GetTick(void);

static uint64_t get_tick_ms(void)
{
    return (uint64_t)HAL_GetTick();
}

#endif

/*=============================================================================
 * Private Data
 *============================================================================*/

static statistics_snapshot_t s_statistics;

static uint64_t s_start_time_ms;

/*=============================================================================
 * Public Functions
 *============================================================================*/

tcp_result_t statistics_initialize(void)
{
    memset(&s_statistics, 0, sizeof(s_statistics));

    s_start_time_ms = get_tick_ms();

    return TCP_RESULT_OK;
}

tcp_result_t statistics_deinitialize(void)
{
    return TCP_RESULT_OK;
}

void statistics_update(
    uint32_t packet_bytes,
    uint32_t payload_bytes)
{
    s_statistics.packet_count++;

    s_statistics.total_bytes +=
        (uint64_t)packet_bytes;

    s_statistics.total_payload_bytes +=
        (uint64_t)payload_bytes;
}

tcp_result_t statistics_get_snapshot(
    statistics_snapshot_t* snapshot)
{
    uint64_t elapsed;
    statistics_snapshot_t current;

    if (snapshot == NULL)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    current = s_statistics;

    elapsed =
        get_tick_ms() -
        s_start_time_ms;

    current.elapsed_time_ms = elapsed;

    if (elapsed > 0ULL)
    {
        current.packets_per_second =
            (uint32_t)
            ((current.packet_count * 1000ULL) /
             elapsed);

        current.bytes_per_second =
            (uint32_t)
            ((current.total_bytes * 1000ULL) /
             elapsed);
    }
    else
    {
        current.packets_per_second = 0U;

        current.bytes_per_second = 0U;
    }

    *snapshot = current;

    return TCP_RESULT_OK;
}