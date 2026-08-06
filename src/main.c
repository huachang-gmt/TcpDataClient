/******************************************************************************

* @file    main.c
* @brief   TcpDataClient application entry point
*
* Copyright (c) 2026
*
* This file is the application entry point for the Windows simulator.
*
* Responsibilities:
* ```
   - Initialize logger
  ```
* ```
   - Build TcpClient configuration
  ```
* ```
   - Initialize TcpClient
  ```
* ```
   - Execute TcpClient processing loop
  ```
* ```
   - Deinitialize TcpClient
  ```
* ```
   - Deinitialize logger
  ```
*
* This file does NOT:
* ```
   - Access sockets directly
  ```
* ```
   - Build packets
  ```
* ```
   - Generate payload
  ```
* ```
   - Manage TCP connection
  ```
* ```
   - Manage transmission timing
  ```
* ```
   - Manage statistics
  ```
*
* The application architecture shall remain compatible with
* the future STM32H755 implementation.
*

******************************************************************************/

#include "common.h"
#include "config.h"
#include "logger.h"
#include "tcp_client.h"
#include "statistics.h"
#include <stdio.h>

#if TCP_PLATFORM_WINDOWS

#include <windows.h>

#endif

/*=============================================================================
 * Application Control
 *============================================================================*/

#if TCP_PLATFORM_WINDOWS

static volatile BOOL s_stop_requested = FALSE;


/**
 * @brief Windows console control handler.
 *
 * Ctrl+C requests a graceful application shutdown.
 */
static BOOL WINAPI console_control_handler(
    DWORD control_type)
{
    if (control_type == CTRL_C_EVENT)
    {
        s_stop_requested = TRUE;

        return TRUE;
    }

    return FALSE;
}

#endif


/*=============================================================================

* Private Functions
  *============================================================================*/

/**

* @brief Build TcpClient configuration.
*
* @param[out] config
* 
   TcpClient configuration.  
*
* @return TCP_RESULT_OK if successful.
  */
  static tcp_result_t build_client_config(
  tcp_client_config_t* config)
  {
  if (config == NULL)
  {
  return TCP_RESULT_INVALID_PARAMETER;
  }

  config->server_ip =
  TCP_DEFAULT_SERVER_IP;

  config->server_port =
  (uint16_t)TCP_DEFAULT_SERVER_PORT;

  config->payload_length =
  TCP_DEFAULT_PAYLOAD_LENGTH;

  config->send_interval_ms =
  TCP_DEFAULT_SEND_INTERVAL_MS;

  config->packet_count =
  TCP_DEFAULT_PACKET_COUNT;

  return TCP_RESULT_OK;
  }




/**

* @brief Print periodic transmission statistics.
*
* This function is called approximately once per second.
*
* The function intentionally does not print every packet because
* console output can affect the timing of the Windows simulator.
*
* @param[in,out] last_time_ms
* ```
   Previous statistics print timestamp.
  ```
*
* @param[in,out] last_packet_count
* ```
   Previous packet count.
  ```
*
* @param[in,out] process_loop_count
* ```
   Main processing loop counter.
  ```

*/

#if TCP_PLATFORM_WINDOWS

static void print_periodic_statistics(
uint64_t* last_time_ms,
uint64_t* last_packet_count,
uint64_t process_loop_count)
{
uint64_t current_time_ms;
uint64_t elapsed_ms;

statistics_snapshot_t snapshot;

tcp_result_t result;

char message[256];

uint64_t packet_delta;

if ((last_time_ms == NULL) ||
    (last_packet_count == NULL))
{
    return;
}

current_time_ms =
    (uint64_t)GetTickCount64();

elapsed_ms =
    current_time_ms - *last_time_ms;

/*
 * Print approximately once per second.
 */
if (elapsed_ms < 1000ULL)
{
    return;
}

/*
 * Get current transmission statistics.
 */
result =
    statistics_get_snapshot(
        &snapshot);

if (result != TCP_RESULT_OK)
{
    logger_error(
        "[STAT] Failed to get statistics.");

    *last_time_ms =
        current_time_ms;

    return;
}




/*
 * Calculate packets transmitted during
 * the previous statistics interval.
 */
packet_delta =
    snapshot.packet_count -
    *last_packet_count;


/*
 * Main loop diagnostic.
 */
(void)snprintf(
    message,
    sizeof(message),
    "[DEBUG] MainLoop=%llu State=%d",
    (unsigned long long)process_loop_count,
    (int)tcp_client_get_state());

logger_debug(
    message);


/*
 * Total transmission statistics.
 */
(void)snprintf(
    message,
    sizeof(message),
    "[STAT] Packets=%llu Delta=%llu Payload=%llu Total=%llu",
    (unsigned long long)snapshot.packet_count,
    (unsigned long long)packet_delta,
    (unsigned long long)snapshot.total_payload_bytes,
    (unsigned long long)snapshot.total_bytes);

logger_info(
    message);


/*
 * Throughput.
 */
(void)snprintf(
    message,
    sizeof(message),
    "[STAT] Rate=%lu pkt/s  %lu bytes/s",
    (unsigned long)snapshot.packets_per_second,
    (unsigned long)snapshot.bytes_per_second);

logger_info(
    message);


/*
 * Explicit transmission activity indication.
 */
if (packet_delta == 0ULL)
{
    logger_warning(
        "[STAT] WARNING: Packet count did not increase.");
}
else
{
    logger_debug(
        "[STAT] TX activity detected.");
}


/*
 * Save current values for the next one-second interval.
 */
*last_packet_count =
    snapshot.packet_count;

*last_time_ms =
    current_time_ms;

}


#endif





/*=============================================================================

* Application Entry Point
  *============================================================================*/

int main(void)
{
tcp_client_config_t config;
tcp_result_t result;

#if TCP_PLATFORM_WINDOWS

uint64_t statistics_time_ms;
uint64_t last_packet_count;
uint64_t process_loop_count;

#endif

#if TCP_PLATFORM_WINDOWS

    if (!SetConsoleCtrlHandler(
            console_control_handler,
            TRUE))
    {
        return 1;
    }

#endif

/*---------------------------------------------------------------------
 * Initialize Logger
 *---------------------------------------------------------------------*/

result = logger_initialize();

if (result != TCP_RESULT_OK)
{
    return 1;
}

logger_info(
    "TcpDataClient starting.");

/*---------------------------------------------------------------------
 * Build Client Configuration
 *---------------------------------------------------------------------*/

result = build_client_config(
    &config);

if (result != TCP_RESULT_OK)
{
    logger_error(
        "Invalid TCP client configuration.");

    logger_deinitialize();

    return 1;
}


/*
 * Print configuration before TcpClient initialization.
 *
 * This confirms that the runtime configuration is exactly
 * what we expect during the test.
 */
{
    char message[256];


    (void)snprintf(
        message,
        sizeof(message),
        "[CONFIG] Server=%s:%u",
        config.server_ip,
        (unsigned int)config.server_port);

    logger_info(
        message);


    (void)snprintf(
        message,
        sizeof(message),
        "[CONFIG] Payload=%u bytes",
        (unsigned int)config.payload_length);

    logger_info(
        message);


    (void)snprintf(
        message,
        sizeof(message),
        "[CONFIG] Interval=%u ms",
        (unsigned int)config.send_interval_ms);

    logger_info(
        message);


    (void)snprintf(
        message,
        sizeof(message),
        "[CONFIG] PacketCount=%llu",
        (unsigned long long)config.packet_count);

    logger_info(
        message);
}


/*---------------------------------------------------------------------
 * Initialize TcpClient
 *---------------------------------------------------------------------*/

result = tcp_client_initialize(
    &config);

if (result != TCP_RESULT_OK)
{
    logger_error(
        "TcpClient initialization failed.");

    logger_deinitialize();

    return 1;
}

logger_info(
    "TcpDataClient initialized.");



#if TCP_PLATFORM_WINDOWS

/*
 * Initialize diagnostic counters.
 *
 * Statistics are sampled once per second.
 * The processing loop counter lets us distinguish between:
 *
 *   1. main loop stopped
 *   2. main loop alive but no packets transmitted
 *   3. packets transmitted normally
 */
statistics_time_ms =
    (uint64_t)GetTickCount64();

last_packet_count =
    0ULL;

process_loop_count =
    0ULL;

#endif


/*---------------------------------------------------------------------
 * Main Processing Loop
 *---------------------------------------------------------------------*/

while ((tcp_client_get_state() !=
        TCP_CLIENT_STATE_ERROR)
#if TCP_PLATFORM_WINDOWS
       && (s_stop_requested == FALSE)
#endif
      )
{

#if TCP_PLATFORM_WINDOWS

    process_loop_count++;

#endif

    result = tcp_client_process();

    if (result == TCP_RESULT_ERROR)
    {
        logger_error(
            "TcpClient processing error.");

        break;
    }

    if (result == TCP_RESULT_INVALID_PARAMETER)
    {
        logger_error(
            "TcpClient invalid parameter.");

        break;
    }


#if TCP_PLATFORM_WINDOWS

    /*
     * Print diagnostic information once per second.
     *
     * Do NOT add Sleep() here yet.
     *
     * The current test is specifically intended to determine
     * whether the existing processing loop itself is responsible
     * for the low transmission rate.
     */
    print_periodic_statistics(
        &statistics_time_ms,
        &last_packet_count,
        process_loop_count);

#endif

}

#if TCP_PLATFORM_WINDOWS

if (s_stop_requested)
{
    logger_info(
        "Ctrl+C received. Stopping TcpDataClient.");

}

#endif

/*---------------------------------------------------------------------
 * Deinitialize TcpClient
 *---------------------------------------------------------------------*/

result = tcp_client_deinitialize();

if (result != TCP_RESULT_OK)
{
    logger_error(
        "TcpClient deinitialization failed.");
}

logger_info(
    "TcpDataClient stopped.");

/*---------------------------------------------------------------------
 * Deinitialize Logger
 *---------------------------------------------------------------------*/

logger_deinitialize();

#if TCP_PLATFORM_WINDOWS

(void)SetConsoleCtrlHandler(
    console_control_handler,
    FALSE);

#endif

return 0;

}
