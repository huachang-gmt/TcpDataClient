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

/*=============================================================================

* Application Entry Point
  *============================================================================*/

int main(void)
{
tcp_client_config_t config;
tcp_result_t result;

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

/*---------------------------------------------------------------------
 * Main Processing Loop
 *---------------------------------------------------------------------*/

while (tcp_client_get_state() !=
       TCP_CLIENT_STATE_ERROR)
{
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
}

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

return 0;

}
