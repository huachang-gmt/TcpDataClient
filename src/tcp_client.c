/******************************************************************************
 * @file    tcp_client.c
 * @brief   TCP client core implementation
 *
 * Copyright (c) 2026
 *
 * TcpClient is the core control module of TcpDataClient.
 *
 * Responsibilities:
 *      - Initialize all dependent modules
 *      - Manage TCP connection state
 *      - Acquire application payload
 *      - Build packet header
 *      - Send packet through socket abstraction
 *      - Retry pending packet when socket is busy
 *      - Manage transmission interval
 *      - Update transmission statistics
 *      - Manage automatic reconnect
 *
 * This module does NOT:
 *      - Access platform socket APIs directly
 *      - Generate application payload itself
 *      - Print console messages
 *      - Allocate dynamic memory
 *
 * Design:
 *      - No dynamic memory
 *      - No STL
 *      - Static packet buffer
 *      - Zero-copy packet construction
 *      - Platform-independent core logic
 *      - Suitable for Windows / Linux / STM32H7
 *
 ******************************************************************************/

#include "tcp_client.h"

#include "config.h"
#include "protocol.h"
#include "socket_if.h"
#include "packet_builder.h"
#include "data_source.h"
#include "statistics.h"

/*=============================================================================
 * Platform Time
 *============================================================================*/

#if TCP_PLATFORM_WINDOWS

#include <windows.h>


/*
 * High-resolution Windows timer.
 *
 * GetTickCount64() has a relatively coarse effective resolution
 * and is therefore unsuitable for the required 1 ms transmission
 * scheduling.
 *
 * QueryPerformanceCounter() provides a high-resolution,
 * monotonic timer suitable for this purpose.
 */

static LARGE_INTEGER s_qpc_frequency;


/**
 * @brief Get high-resolution monotonic time in milliseconds.
 *
 * @return Current monotonic time in milliseconds.
 */
static uint64_t tcp_client_get_time_ms(void)
{
    LARGE_INTEGER counter;

    if (!QueryPerformanceCounter(&counter))
    {
        return 0ULL;
    }

    return
        ((uint64_t)counter.QuadPart * 1000ULL) /
        (uint64_t)s_qpc_frequency.QuadPart;
}


/**
 * @brief Initialize Windows high-resolution timer.
 *
 * @return TRUE if successful.
 */
static bool tcp_client_initialize_timer(void)
{
    if (!QueryPerformanceFrequency(
            &s_qpc_frequency))
    {
        return false;
    }

    return true;
}

#elif TCP_PLATFORM_LINUX

#include <time.h>

static uint64_t tcp_client_get_time_ms(void)
{
    struct timespec ts;

    clock_gettime(
        CLOCK_MONOTONIC,
        &ts);

    return ((uint64_t)ts.tv_sec * 1000ULL) +
           ((uint64_t)ts.tv_nsec / 1000000ULL);
}

#elif TCP_PLATFORM_STM32

extern uint32_t HAL_GetTick(void);

static uint64_t tcp_client_get_time_ms(void)
{
    return (uint64_t)HAL_GetTick();
}

#endif

/*=============================================================================
 * Private Constants
 *============================================================================*/

/*
 * Complete packet buffer.
 *
 * Maximum packet:
 *
 *      Header  = 24 bytes
 *      Payload = 2048 bytes
 *
 * Total:
 *
 *      2072 bytes
 *
 * Static allocation is intentional.
 *
 * No malloc/free is used because the same TcpClient core
 * must be portable to STM32H755.
 */
#define TCP_CLIENT_PACKET_BUFFER_SIZE \
    (TCP_MAX_PACKET_LENGTH)

/*=============================================================================
 * Private Data
 *============================================================================*/

typedef struct
{
    tcp_client_state_t state;

    tcp_client_config_t config;

    socket_handle_t socket;

    /*
     * Complete packet buffer.
     *
     * Layout:
     *
     * +---------------------------+
     * | tcp_packet_header_t       |
     * +---------------------------+
     * | application payload       |
     * +---------------------------+
     */
    uint8_t packet_buffer[
        TCP_CLIENT_PACKET_BUFFER_SIZE];

    uint32_t packet_length;

    uint32_t payload_length;

    /*
     * TRUE:
     *
     * packet_buffer contains a packet that has already been
     * generated and must be sent before generating another packet.
     *
     * This prevents loss of application data when socket_if_send()
     * returns TCP_RESULT_BUSY or the connection temporarily fails.
     */
    bool packet_pending;

    /*
     * Number of packets successfully accepted/sent.
     *
     * This is intentionally separate from packet_builder sequence.
     */
    uint64_t transmitted_packets;

    /*
     * Next scheduled transmission time.
     */
    uint64_t next_send_time_ms;

    /*
     * Next reconnect attempt time.
     */
    uint64_t next_reconnect_time_ms;

    /*
     * Module initialization flags.
     *
     * Used for safe rollback during initialization and
     * safe repeated deinitialization.
     */
    bool socket_initialized;

    bool packet_builder_initialized;

    bool data_source_initialized;

    bool statistics_initialized;

} tcp_client_context_t;


/*
 * Single TcpClient instance.
 *
 * Static allocation is intentional.
 */
static tcp_client_context_t s_client;

/*=============================================================================
 * Private Functions
 *============================================================================*/

/**
 * @brief Reset client context.
 *
 * This function does not initialize external modules.
 */
static void tcp_client_reset_context(void)
{
    s_client.state =
        TCP_CLIENT_STATE_IDLE;

    s_client.config.server_ip =
        NULL;

    s_client.config.server_port =
        0U;

    s_client.config.payload_length =
        0U;

    s_client.config.send_interval_ms =
        0U;

    s_client.config.packet_count =
        0ULL;

    s_client.socket.native_handle =
        SOCKET_HANDLE_INVALID;

    s_client.packet_length =
        0U;

    s_client.payload_length =
        0U;

    s_client.packet_pending =
        false;

    s_client.transmitted_packets =
        0ULL;

    s_client.next_send_time_ms =
        0ULL;

    s_client.next_reconnect_time_ms =
        0ULL;

    s_client.socket_initialized =
        false;

    s_client.packet_builder_initialized =
        false;

    s_client.data_source_initialized =
        false;

    s_client.statistics_initialized =
        false;
}


/**
 * @brief Check whether packet transmission is completed.
 *
 * @return true if configured packet count has been transmitted.
 */
static bool tcp_client_is_transmission_complete(void)
{
    if (s_client.config.packet_count ==
        TCP_PACKET_COUNT_INFINITE)
    {
        return false;
    }

    return
        (s_client.transmitted_packets >=
         s_client.config.packet_count);
}


/**
 * @brief Validate client configuration.
 */
static tcp_result_t tcp_client_validate_config(
    const tcp_client_config_t* config)
{
    if (config == NULL)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if (config->server_ip == NULL)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if (config->server_port == 0U)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if ((config->payload_length == 0U) ||
        (config->payload_length >
         TCP_MAX_PAYLOAD_LENGTH))
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    /*
     * A zero interval is not useful for the intended
     * deterministic 1 ms data acquisition model.
     *
     * It is therefore rejected rather than creating a
     * busy-loop configuration.
     */
    if (config->send_interval_ms == 0U)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    return TCP_RESULT_OK;
}


/**
 * @brief Generate one complete packet.
 *
 * Data Source generates payload first.
 *
 * Packet Builder then writes the header into the same
 * packet buffer.
 *
 * No second packet buffer is required.
 */
static tcp_result_t tcp_client_build_packet(void)
{
    tcp_result_t result;

    uint8_t* payload;

    uint32_t payload_length;

    payload =
        TCP_PACKET_PAYLOAD(
            s_client.packet_buffer);

    result =
        data_source_read(
            payload,
            TCP_MAX_PAYLOAD_LENGTH,
            &payload_length);

    if (result != TCP_RESULT_OK)
    {
        return result;
    }

    if ((payload_length == 0U) ||
        (payload_length >
         TCP_MAX_PAYLOAD_LENGTH))
    {
        return TCP_RESULT_ERROR;
    }

    /*
     * Data Source owns payload generation.
     *
     * TcpClient verifies that the source obeyed
     * the configured payload size.
     */
    if (payload_length !=
        s_client.config.payload_length)
    {
        return TCP_RESULT_ERROR;
    }

    result =
        packet_builder_build(
            TCP_PACKET_HEADER(
                s_client.packet_buffer),
            payload_length);

    if (result != TCP_RESULT_OK)
    {
        return result;
    }

    s_client.payload_length =
        payload_length;

    s_client.packet_length =
        TCP_PACKET_HEADER_SIZE +
        payload_length;

    /*
     * The packet is now complete.
     *
     * It must remain unchanged until socket_if_send()
     * successfully accepts/sends it.
     */
    s_client.packet_pending =
        true;

    return TCP_RESULT_OK;
}


/**
 * @brief Send the currently pending packet.
 *
 * This function never generates another packet.
 *
 * This is important because a BUSY result must not cause
 * the Data Source to advance to the next sample.
 */
static tcp_result_t tcp_client_send_pending_packet(void)
{
    tcp_result_t result;

    if (!s_client.packet_pending)
    {
        return TCP_RESULT_OK;
    }

    result =
        socket_if_send(
            s_client.socket,
            s_client.packet_buffer,
            s_client.packet_length);

    if (result == TCP_RESULT_OK)
    {
        /*
         * The complete packet has now been accepted/sent.
         *
         * Only at this point is it considered transmitted.
         */
        statistics_update(
            s_client.packet_length,
            s_client.payload_length);

        ++s_client.transmitted_packets;

        s_client.packet_pending =
            false;

        s_client.packet_length =
            0U;

        s_client.payload_length =
            0U;

        return TCP_RESULT_OK;
    }

    /*
     * TCP_RESULT_BUSY:
     *
     * Keep packet_pending == true.
     *
     * The exact same packet will be retried later.
     */
    if (result == TCP_RESULT_BUSY)
    {
        return TCP_RESULT_BUSY;
    }

    /*
     * Any connection failure means the current packet
     * must not be discarded.
     *
     * The packet remains in packet_buffer and will be
     * retried after reconnection.
     */
    if (result == TCP_RESULT_DISCONNECTED)
    {
        socket_if_disconnect(
            &s_client.socket);

        s_client.state =
            TCP_CLIENT_STATE_DISCONNECTED;

        s_client.next_reconnect_time_ms =
            tcp_client_get_time_ms() +
            (uint64_t)TCP_RECONNECT_INTERVAL_MS;

        return TCP_RESULT_DISCONNECTED;
    }

    /*
     * Other socket errors are treated as fatal for the
     * current client instance.
     *
     * The packet remains in memory so it is not silently
     * destroyed.
     */
    s_client.state =
        TCP_CLIENT_STATE_ERROR;

    return result;
}


/**
 * @brief Establish TCP connection.
 */
static tcp_result_t tcp_client_connect(void)
{
    socket_config_t socket_config;

    tcp_result_t result;

    socket_config.server_ip =
        s_client.config.server_ip;

    socket_config.server_port =
        s_client.config.server_port;

    s_client.state =
        TCP_CLIENT_STATE_CONNECTING;

    result =
        socket_if_connect(
            &s_client.socket,
            &socket_config);

    if (result == TCP_RESULT_OK)
    {
        s_client.state =
            TCP_CLIENT_STATE_CONNECTED;

        /*
         * Send immediately after connection.
         */
        s_client.next_send_time_ms =
            tcp_client_get_time_ms();

        return TCP_RESULT_OK;
    }

    s_client.state =
        TCP_CLIENT_STATE_DISCONNECTED;

    s_client.next_reconnect_time_ms =
        tcp_client_get_time_ms() +
        (uint64_t)TCP_RECONNECT_INTERVAL_MS;

    return result;
}


/**
 * @brief Handle disconnected state.
 */
static tcp_result_t tcp_client_process_disconnected(void)
{
    uint64_t now;

    if (!TCP_ENABLE_AUTO_RECONNECT)
    {
        return TCP_RESULT_DISCONNECTED;
    }

    now =
        tcp_client_get_time_ms();

    if (now <
        s_client.next_reconnect_time_ms)
    {
        return TCP_RESULT_BUSY;
    }

    return tcp_client_connect();
}


/**
 * @brief Handle connected/running state.
 */
static tcp_result_t tcp_client_process_running(void)
{
    uint64_t now;

    tcp_result_t result;

    /*
     * If a previous packet has not been successfully
     * transmitted, retry that packet first.
     */
    if (s_client.packet_pending)
    {
        result =
            tcp_client_send_pending_packet();

        if (result == TCP_RESULT_BUSY)
        {
            return TCP_RESULT_BUSY;
        }

        if (result == TCP_RESULT_DISCONNECTED)
        {
            return TCP_RESULT_DISCONNECTED;
        }

        if (result != TCP_RESULT_OK)
        {
            return result;
        }

        /*
         * Packet was successfully transmitted.
         *
         * If packet count is complete, stop generating
         * additional packets.
         */
        if (tcp_client_is_transmission_complete())
        {
            socket_if_disconnect(
                &s_client.socket);

            s_client.state =
                TCP_CLIENT_STATE_INITIALIZED;

            return TCP_RESULT_OK;
        }

        /*
         * Continue with normal transmission scheduling.
         */
        now =
            tcp_client_get_time_ms();

        /*
         * Maintain periodic schedule rather than using
         *
         *     next = now + interval
         *
         * This prevents long-term timing drift.
         */
        do
        {
            s_client.next_send_time_ms +=
                (uint64_t)
                s_client.config.send_interval_ms;

        } while (s_client.next_send_time_ms <= now);

        return TCP_RESULT_OK;
    }

    now =
        tcp_client_get_time_ms();

    if (now <
        s_client.next_send_time_ms)
    {
        return TCP_RESULT_BUSY;
    }

    /*
     * Generate the next packet only when the previous
     * packet is no longer pending.
     */
    result =
        tcp_client_build_packet();

    if (result != TCP_RESULT_OK)
    {
        s_client.state =
            TCP_CLIENT_STATE_ERROR;

        return result;
    }

    /*
     * Attempt transmission immediately.
     */
    result =
        tcp_client_send_pending_packet();

    if (result == TCP_RESULT_BUSY)
    {
        return TCP_RESULT_BUSY;
    }

    if (result == TCP_RESULT_DISCONNECTED)
    {
        return TCP_RESULT_DISCONNECTED;
    }

    if (result != TCP_RESULT_OK)
    {
        return result;
    }

    /*
     * Transmission succeeded.
     */
    if (tcp_client_is_transmission_complete())
    {
        socket_if_disconnect(
            &s_client.socket);

        s_client.state =
            TCP_CLIENT_STATE_INITIALIZED;

        return TCP_RESULT_OK;
    }

    /*
     * Schedule next packet from the previous deadline.
     *
     * This preserves the requested periodic transmission
     * model even when tcp_client_process() is called
     * irregularly.
     */
    now =
        tcp_client_get_time_ms();

    do
    {
        s_client.next_send_time_ms +=
            (uint64_t)
            s_client.config.send_interval_ms;

    } while (s_client.next_send_time_ms <= now);

    return TCP_RESULT_OK;
}


/*=============================================================================
 * Public Functions
 *============================================================================*/

tcp_result_t tcp_client_initialize(
    const tcp_client_config_t* config)
{
    tcp_result_t result;

    /*
     * Validate configuration before changing
     * any external module state.
     */
    result =
        tcp_client_validate_config(config);

    if (result != TCP_RESULT_OK)
    {
        return result;
    }

#if TCP_PLATFORM_WINDOWS

    /*
     * Initialize high-resolution timer used by
     * transmission scheduling.
     */
    if (!tcp_client_initialize_timer())
    {
        return TCP_RESULT_ERROR;
    }

#endif


    /*
     * Always start from a clean context.
     */
    tcp_client_reset_context();

    /*
     * Store configuration.
     *
     * server_ip is intentionally stored as a pointer.
     *
     * Caller must keep the referenced string valid
     * until tcp_client_deinitialize().
     */
    s_client.config =
        *config;

    /*
     * Initialize socket subsystem.
     */
    result =
        socket_if_initialize();

    if (result != TCP_RESULT_OK)
    {
        tcp_client_reset_context();

        return result;
    }

    s_client.socket_initialized =
        true;

    /*
     * Initialize Packet Builder.
     */
    result =
        packet_builder_initialize();

    if (result != TCP_RESULT_OK)
    {
        goto initialize_error;
    }

    s_client.packet_builder_initialized =
        true;

    /*
     * Initialize Data Source.
     */
    {
        data_source_config_t data_config;

        data_config.source_type =
            DATA_SOURCE_SIMULATOR;

        data_config.payload_length =
            config->payload_length;

        result =
            data_source_initialize(
                &data_config);
    }

    if (result != TCP_RESULT_OK)
    {
        goto initialize_error;
    }

    s_client.data_source_initialized =
        true;

    /*
     * Initialize Statistics.
     */
    result =
        statistics_initialize();

    if (result != TCP_RESULT_OK)
    {
        goto initialize_error;
    }

    s_client.statistics_initialized =
        true;

    /*
     * Client is now fully initialized.
     */
    s_client.state =
        TCP_CLIENT_STATE_INITIALIZED;

    /*
     * Establish initial connection.
     *
     * Connection is performed here rather than waiting
     * for the first process cycle.
     */
    result =
        tcp_client_connect();

    if ((result != TCP_RESULT_OK) &&
        (result != TCP_RESULT_DISCONNECTED))
    {
        goto initialize_error;
    }

    /*
     * A disconnected initial connection is not treated
     * as fatal when automatic reconnect is enabled.
     *
     * tcp_client_process() will perform the retry.
     */
    if (result == TCP_RESULT_DISCONNECTED)
    {
        return TCP_RESULT_OK;
    }

    return TCP_RESULT_OK;


/*=============================================================================
 * Initialization Rollback
 *============================================================================*/

initialize_error:

    if (s_client.statistics_initialized)
    {
        statistics_deinitialize();

        s_client.statistics_initialized =
            false;
    }

    if (s_client.data_source_initialized)
    {
        data_source_deinitialize();

        s_client.data_source_initialized =
            false;
    }

    if (s_client.packet_builder_initialized)
    {
        packet_builder_deinitialize();

        s_client.packet_builder_initialized =
            false;
    }

    if (s_client.socket_initialized)
    {
        socket_if_disconnect(
            &s_client.socket);

        socket_if_deinitialize();

        s_client.socket_initialized =
            false;
    }

    s_client.state =
        TCP_CLIENT_STATE_ERROR;

    return result;
}


tcp_result_t tcp_client_process(void)
{
    socket_state_t socket_state;

    /*
     * Process must only operate after initialization.
     */
    if ((s_client.state ==
         TCP_CLIENT_STATE_IDLE) ||
        (s_client.state ==
         TCP_CLIENT_STATE_ERROR))
    {
        return TCP_RESULT_ERROR;
    }

    /*
     * Transmission has completed.
     *
     * INITIALIZED means the client is still valid but
     * has no active transmission.
     */
    if (s_client.state ==
        TCP_CLIENT_STATE_INITIALIZED)
    {
        return TCP_RESULT_OK;
    }

    /*
     * Handle disconnected state first.
     */
    if (s_client.state ==
        TCP_CLIENT_STATE_DISCONNECTED)
    {
        return tcp_client_process_disconnected();
    }

    /*
     * CONNECTING should normally not persist because
     * socket_if_connect() is synchronous in the current
     * interface.
     */
    if (s_client.state ==
        TCP_CLIENT_STATE_CONNECTING)
    {
        return TCP_RESULT_BUSY;
    }

    /*
     * Verify the platform socket state before attempting
     * packet transmission.
     */
    socket_state =
        socket_if_get_state(
            s_client.socket);

    if (socket_state !=
        SOCKET_STATE_CONNECTED)
    {
        socket_if_disconnect(
            &s_client.socket);

        s_client.state =
            TCP_CLIENT_STATE_DISCONNECTED;

        s_client.next_reconnect_time_ms =
            tcp_client_get_time_ms() +
            (uint64_t)TCP_RECONNECT_INTERVAL_MS;

        return TCP_RESULT_DISCONNECTED;
    }

    /*
     * Socket is connected.
     */
    s_client.state =
        TCP_CLIENT_STATE_RUNNING;

    return tcp_client_process_running();
}


tcp_result_t tcp_client_deinitialize(void)
{
    /*
     * Disconnect first so that no platform socket
     * remains active while lower-level modules are
     * being deinitialized.
     */
    if (s_client.socket_initialized)
    {
        socket_if_disconnect(
            &s_client.socket);
    }

    if (s_client.statistics_initialized)
    {
        statistics_deinitialize();

        s_client.statistics_initialized =
            false;
    }

    if (s_client.data_source_initialized)
    {
        data_source_deinitialize();

        s_client.data_source_initialized =
            false;
    }

    if (s_client.packet_builder_initialized)
    {
        packet_builder_deinitialize();

        s_client.packet_builder_initialized =
            false;
    }

    if (s_client.socket_initialized)
    {
        socket_if_deinitialize();

        s_client.socket_initialized =
            false;
    }

    /*
     * Return the client to the initial state.
     */
    tcp_client_reset_context();

    return TCP_RESULT_OK;
}


tcp_client_state_t tcp_client_get_state(void)
{
    return s_client.state;
}
