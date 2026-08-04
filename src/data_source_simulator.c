/******************************************************************************
 * @file    data_source_simulator.c
 * @brief   Simulator data source
 *
 * Copyright (c) 2026
 *
 * This module simulates an EtherCAT data source.
 *
 * Responsibilities:
 *      - Generate application payload
 *      - Produce deterministic test pattern
 *
 * This module does NOT:
 *      - Build packet header
 *      - Manage sequence number
 *      - Send TCP packet
 *
 ******************************************************************************/

#include "data_source.h"

#include "config.h"

#include <string.h>

/*=============================================================================
 * Private Data
 *============================================================================*/

static uint32_t s_packet_index = 0U;
static uint32_t s_payload_length = 0U;

/*=============================================================================
 * Public Functions
 *============================================================================*/

tcp_result_t data_source_initialize(
    const data_source_config_t* config)
{
    if (config == NULL)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if ((config->payload_length == 0U) ||
        (config->payload_length > TCP_MAX_PAYLOAD_LENGTH))
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    s_packet_index = 0U;

    s_payload_length = config->payload_length;

    return TCP_RESULT_OK;
}

tcp_result_t data_source_deinitialize(void)
{
    return TCP_RESULT_OK;
}

tcp_result_t data_source_read(
    uint8_t* payload,
    uint32_t payload_capacity,
    uint32_t* payload_length)
{
    uint32_t i;

    if ((payload == NULL) ||
        (payload_length == NULL))
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if (s_payload_length == 0U)
    {
        return TCP_RESULT_ERROR;
    }

    if (payload_capacity < s_payload_length)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    *payload_length = s_payload_length;

    for (i = 0U; i < s_payload_length; ++i)
    {
        payload[i] = (uint8_t)((s_packet_index + i) & 0xFFU);
    }

    ++s_packet_index;

    return TCP_RESULT_OK;
}