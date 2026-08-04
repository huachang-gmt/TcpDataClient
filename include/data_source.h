/******************************************************************************
 * @file    data_source.h
 * @brief   Application payload provider
 *
 * Copyright (c) 2026
 *
 * Data Source is responsible for providing application payload only.
 *
 * Responsibilities:
 *      - Generate or acquire application payload
 *      - Fill payload buffer
 *
 * Data Source does NOT know:
 *      - TCP
 *      - Socket
 *      - Packet Header
 *      - Sequence Number
 *
 * Current Supported Source:
 *      - Simulator
 *
 * Future Supported Sources:
 *      - EtherCAT
 *      - File Replay
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_DATA_SOURCE_H
#define TCP_DATA_CLIENT_DATA_SOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"

/*=============================================================================
 * Data Source Type
 *============================================================================*/

typedef enum
{
    DATA_SOURCE_SIMULATOR = 0,

    DATA_SOURCE_FILE,

    DATA_SOURCE_ETHERCAT

} data_source_type_t;

/*=============================================================================
 * Configuration
 *============================================================================*/

typedef struct
{
    data_source_type_t source_type;

    uint32_t payload_length;

} data_source_config_t;

/*=============================================================================
 * Interface
 *============================================================================*/

/**
 * @brief Initialize data source.
 *
 * @param[in] config
 *      Data source configuration.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t data_source_initialize(
    const data_source_config_t* config);

/**
 * @brief Deinitialize data source.
 *
 * @return TCP_RESULT_OK
 */
tcp_result_t data_source_deinitialize(void);

/**
 * @brief Read one application payload.
 *
 * This function fills one application payload.
 *
 * Packet Header is NOT included.
 *
 * @param[out] payload
 *      Payload buffer.
 *
 * @param[in] payload_capacity
 *      Maximum writable payload size.
 *
 * @param[out] payload_length
 *      Actual payload length.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t data_source_read(
    uint8_t* payload,
    uint32_t payload_capacity,
    uint32_t* payload_length);

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_DATA_SOURCE_H */