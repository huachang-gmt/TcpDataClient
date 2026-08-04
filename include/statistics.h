/******************************************************************************
 * @file    statistics.h
 * @brief   Transmission statistics
 *
 * Copyright (c) 2026
 *
 * Statistics module records transmission information.
 *
 * Responsibilities:
 *      - Count transmitted packets
 *      - Count transmitted bytes
 *      - Measure elapsed time
 *      - Calculate throughput
 *
 * This module does NOT:
 *      - Print console messages
 *      - Know socket status
 *      - Allocate memory
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_STATISTICS_H
#define TCP_DATA_CLIENT_STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"

/*=============================================================================
 * Statistics Snapshot
 *============================================================================*/

typedef struct
{
    uint64_t packet_count;

    uint64_t total_bytes;

    uint64_t total_payload_bytes;

    uint64_t elapsed_time_ms;

    uint32_t packets_per_second;

    uint32_t bytes_per_second;

} statistics_snapshot_t;

/*=============================================================================
 * Interface
 *============================================================================*/

/**
 * @brief Initialize statistics module.
 */
tcp_result_t statistics_initialize(void);

/**
 * @brief Deinitialize statistics module.
 */
tcp_result_t statistics_deinitialize(void);

/**
 * @brief Update statistics after one packet is sent.
 *
 * @param packet_bytes
 *      Total packet length (Header + Payload).
 *
 * @param payload_bytes
 *      Payload length.
 */
void statistics_update(
    uint32_t packet_bytes,
    uint32_t payload_bytes);

/**
 * @brief Get current statistics snapshot.
 *
 * @param[out] snapshot
 */
tcp_result_t statistics_get_snapshot(
    statistics_snapshot_t* snapshot);

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_STATISTICS_H */