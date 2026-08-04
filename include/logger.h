/******************************************************************************
 * @file    logger.h
 * @brief   Console logging interface
 *
 * Copyright (c) 2026
 *
 * Logger is responsible for application console output only.
 *
 * Responsibilities:
 *      - Print application information
 *      - Print warning messages
 *      - Print error messages
 *      - Print debug messages
 *
 * Logger does NOT:
 *      - Manage TCP connections
 *      - Generate packets
 *      - Access sockets
 *      - Generate payload
 *      - Manage statistics
 *      - Allocate dynamic memory
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_LOGGER_H
#define TCP_DATA_CLIENT_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"

/*=============================================================================
 * Logger Level
 *============================================================================*/

typedef enum
{
    LOGGER_LEVEL_INFO = 0,

    LOGGER_LEVEL_WARNING,

    LOGGER_LEVEL_ERROR,

    LOGGER_LEVEL_DEBUG

} logger_level_t;

/*=============================================================================
 * Interface
 *============================================================================*/

/**
 * @brief Initialize logger.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t logger_initialize(void);


/**
 * @brief Deinitialize logger.
 *
 * @return TCP_RESULT_OK.
 */
tcp_result_t logger_deinitialize(void);


/**
 * @brief Print information message.
 *
 * @param[in] message
 *      Null-terminated message string.
 */
void logger_info(
    const char* message);


/**
 * @brief Print warning message.
 *
 * @param[in] message
 *      Null-terminated message string.
 */
void logger_warning(
    const char* message);


/**
 * @brief Print error message.
 *
 * @param[in] message
 *      Null-terminated message string.
 */
void logger_error(
    const char* message);


/**
 * @brief Print debug message.
 *
 * @param[in] message
 *      Null-terminated message string.
 */
void logger_debug(
    const char* message);

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_LOGGER_H */
