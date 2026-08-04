/******************************************************************************
 * @file    tcp_client.h
 * @brief   TCP client interface
 *
 * Copyright (c) 2026
 *
 * TcpClient is the core module of TcpDataClient.
 *
 * Responsibilities:
 *      - Initialize all modules
 *      - Manage TCP connection
 *      - Build packets
 *      - Send packets
 *      - Update statistics
 *      - Manage client state
 *
 * TcpClient does NOT:
 *      - Generate payload data
 *      - Access platform socket APIs directly
 *      - Print console messages
 *      - Allocate dynamic memory
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_TCP_CLIENT_H
#define TCP_DATA_CLIENT_TCP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"

/*=============================================================================
 * Client State
 *============================================================================*/

typedef enum
{
    TCP_CLIENT_STATE_IDLE = 0,

    TCP_CLIENT_STATE_INITIALIZED,

    TCP_CLIENT_STATE_CONNECTING,

    TCP_CLIENT_STATE_CONNECTED,

    TCP_CLIENT_STATE_RUNNING,

    TCP_CLIENT_STATE_DISCONNECTED,

    TCP_CLIENT_STATE_ERROR

} tcp_client_state_t;

/*=============================================================================
 * Client Configuration
 *============================================================================*/

typedef struct
{
    const char* server_ip;

    uint16_t server_port;

    uint32_t payload_length;

    uint32_t send_interval_ms;

    uint64_t packet_count;

} tcp_client_config_t;

/*=============================================================================
 * Interface
 *============================================================================*/

/**
 * @brief Initialize TCP client.
 *
 * This function initializes:
 *
 *      - Socket layer
 *      - Packet builder
 *      - Data source
 *      - Statistics
 *
 * @param[in] config
 *      Client configuration.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t tcp_client_initialize(
    const tcp_client_config_t* config);

/**
 * @brief Execute one client cycle.
 *
 * This function is designed to be called repeatedly.
 *
 * Windows:
 *
 *      while (...)
 *      {
 *          tcp_client_process();
 *      }
 *
 * STM32:
 *
 *      while (1)
 *      {
 *          tcp_client_process();
 *      }
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t tcp_client_process(void);

/**
 * @brief Deinitialize TCP client.
 *
 * Safe to call multiple times.
 *
 * @return TCP_RESULT_OK
 */
tcp_result_t tcp_client_deinitialize(void);

/**
 * @brief Get current client state.
 *
 * @return Current state.
 */
tcp_client_state_t tcp_client_get_state(void);

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_TCP_CLIENT_H */