/******************************************************************************
 * @file    logger.c
 * @brief   Console logging implementation
 *
 * Copyright (c) 2026
 *
 * Platform-independent logger interface.
 *
 ******************************************************************************/

#include "logger.h"

#include <stdio.h>

/*=============================================================================
 * Private Data
 *============================================================================*/

static bool s_initialized = false;

/*=============================================================================
 * Private Functions
 *============================================================================*/

static void logger_print(
    const char* level,
    const char* message)
{
    if ((level == NULL) ||
        (message == NULL))
    {
        return;
    }

    (void)printf(
        "[%s] %s\n",
        level,
        message);
}

/*=============================================================================
 * Public Functions
 *============================================================================*/

tcp_result_t logger_initialize(void)
{
    s_initialized = true;

    return TCP_RESULT_OK;
}


tcp_result_t logger_deinitialize(void)
{
    s_initialized = false;

    return TCP_RESULT_OK;
}


void logger_info(
    const char* message)
{
    if (!s_initialized)
    {
        return;
    }

    logger_print(
        "INFO",
        message);
}


void logger_warning(
    const char* message)
{
    if (!s_initialized)
    {
        return;
    }

    logger_print(
        "WARNING",
        message);
}


void logger_error(
    const char* message)
{
    if (!s_initialized)
    {
        return;
    }

    logger_print(
        "ERROR",
        message);
}


void logger_debug(
    const char* message)
{
    if (!s_initialized)
    {
        return;
    }

    logger_print(
        "DEBUG",
        message);
}

