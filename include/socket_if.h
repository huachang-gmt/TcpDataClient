/******************************************************************************
 * @file    socket_if.h
 * @brief   Platform-independent TCP socket interface
 *
 * Copyright (c) 2026
 *
 * This file defines the abstract socket interface used by TcpDataClient.
 *
 * Platform implementations:
 *
 *     Windows  -> socket_win32.c
 *     Linux    -> socket_linux.c
 *     STM32    -> socket_lwip.c
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_SOCKET_IF_H
#define TCP_DATA_CLIENT_SOCKET_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"

/*=============================================================================
 * Socket Handle
 *============================================================================*/

/*
 * Platform-independent socket handle.
 *
 * Actual implementation is hidden inside platform layer.
 *
 * Windows:
 *      SOCKET
 *
 * Linux:
 *      file descriptor
 *
 * STM32:
 *      struct tcp_pcb*
 */
typedef struct
{
    void* native_handle;

} socket_handle_t;


#define SOCKET_HANDLE_INVALID   NULL


/*=============================================================================
 * Socket State
 *============================================================================*/

typedef enum
{
    SOCKET_STATE_CLOSED = 0,

    SOCKET_STATE_CONNECTING,

    SOCKET_STATE_CONNECTED,

    SOCKET_STATE_ERROR

} socket_state_t;


/*=============================================================================
 * Socket Configuration
 *============================================================================*/

typedef struct
{
    const char* server_ip;

    uint16_t server_port;

} socket_config_t;


/*=============================================================================
 * Interface
 *============================================================================*/

/**
 * @brief Initialize socket subsystem.
 *
 * Windows:
 *      WSAStartup()
 *
 * Linux:
 *      No operation.
 *
 * STM32:
 *      No operation.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t socket_if_initialize(void);


/**
 * @brief Deinitialize socket subsystem.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t socket_if_deinitialize(void);


/**
 * @brief Connect to TCP server.
 *
 * @param[out] handle
 *      Socket handle.
 *
 * @param[in] config
 *      Connection configuration.
 *
 * @return TCP_RESULT_OK if connected.
 */
tcp_result_t socket_if_connect(
    socket_handle_t* handle,
    const socket_config_t* config);


/**
 * @brief Disconnect socket.
 *
 * Safe to call multiple times.
 *
 * @param[in,out] handle
 *
 * @return TCP_RESULT_OK
 */
tcp_result_t socket_if_disconnect(
    socket_handle_t* handle);


/**
 * @brief Send data.
 *
 * Sends one TCP buffer.
 *
 * This layer does not know packet format.
 *
 * The function must not modify the source buffer.
 *
 * Return semantics:
 *
 *      TCP_RESULT_OK
 *          Entire buffer has been accepted/sent.
 *
 *      TCP_RESULT_BUSY
 *          Data cannot be accepted at this time.
 *          Caller should retry later.
 *
 *      TCP_RESULT_DISCONNECTED
 *          TCP connection is no longer valid.
 *
 *      TCP_RESULT_ERROR
 *          Other socket error.
 *
 * @param[in] handle
 *
 * @param[in] buffer
 *
 * @param[in] length
 *
 * @return TCP_RESULT_OK if the complete buffer was sent.
 */
tcp_result_t socket_if_send(
    socket_handle_t handle,
    const uint8_t* buffer,
    uint32_t length);


/**
 * @brief Get current socket state.
 *
 * @param[in] handle
 *
 * @return Current socket state.
 */
socket_state_t socket_if_get_state(
    socket_handle_t handle);


#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_SOCKET_IF_H */