/******************************************************************************
 * @file    packet_builder.c
 * @brief   TCP packet builder
 *
 * Copyright (c) 2026
 *
 * Responsibilities:
 *      - Initialize packet builder
 *      - Generate packet header
 *      - Maintain packet sequence number
 *
 * This module does NOT:
 *      - Allocate memory
 *      - Generate payload
 *      - Send TCP packet
 *
 ******************************************************************************/

#include "packet_builder.h"
#include "config.h"

/*=============================================================================
 * Private Data
 *============================================================================*/

/*
 * Packet sequence number.
 *
 * Incremented once for every packet generated.
 */
static uint64_t s_sequence = 0ULL;

static void packet_builder_write_u16_le(
    uint8_t* destination,
    uint16_t value)
{
    destination[0] = (uint8_t)(value & 0xFFU);
    destination[1] = (uint8_t)((value >> 8) & 0xFFU);
}


static void packet_builder_write_u32_le(
    uint8_t* destination,
    uint32_t value)
{
    destination[0] = (uint8_t)(value & 0xFFU);
    destination[1] = (uint8_t)((value >> 8) & 0xFFU);
    destination[2] = (uint8_t)((value >> 16) & 0xFFU);
    destination[3] = (uint8_t)((value >> 24) & 0xFFU);
}


static void packet_builder_write_u64_le(
    uint8_t* destination,
    uint64_t value)
{
    destination[0] = (uint8_t)(value & 0xFFULL);
    destination[1] = (uint8_t)((value >> 8) & 0xFFULL);
    destination[2] = (uint8_t)((value >> 16) & 0xFFULL);
    destination[3] = (uint8_t)((value >> 24) & 0xFFULL);
    destination[4] = (uint8_t)((value >> 32) & 0xFFULL);
    destination[5] = (uint8_t)((value >> 40) & 0xFFULL);
    destination[6] = (uint8_t)((value >> 48) & 0xFFULL);
    destination[7] = (uint8_t)((value >> 56) & 0xFFULL);
}

/*=============================================================================
 * Public Functions
 *============================================================================*/

tcp_result_t packet_builder_initialize(void)
{
    s_sequence = 0ULL;

    return TCP_RESULT_OK;
}

tcp_result_t packet_builder_deinitialize(void)
{
    return TCP_RESULT_OK;
}

tcp_result_t packet_builder_build(
    tcp_packet_header_t* header,
    uint32_t payload_length)
{
    uint8_t* buffer;

    if (header == NULL)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if ((payload_length == 0U) ||
        (payload_length > TCP_MAX_PAYLOAD_LENGTH))
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    buffer = (uint8_t*)header;

    /*
     * Packet wire format is Little-Endian.
     *
     * Header layout:
     *
     * Offset  Size  Field
     * ------  ----  ----------------
     * 0       2     Version
     * 2       2     Header Size
     * 4       8     Sequence
     * 12      4     Payload Length
     * 16      4     Flags
     * 20      4     Reserved
     */

    packet_builder_write_u16_le(
        &buffer[0],
        TCP_PACKET_VERSION);

    packet_builder_write_u16_le(
        &buffer[2],
        (uint16_t)TCP_PACKET_HEADER_SIZE);

    packet_builder_write_u64_le(
        &buffer[4],
        s_sequence);

    packet_builder_write_u32_le(
        &buffer[12],
        payload_length);

    packet_builder_write_u32_le(
        &buffer[16],
        TCP_PACKET_FLAG_NONE);

    packet_builder_write_u32_le(
        &buffer[20],
        0U);

    ++s_sequence;

    return TCP_RESULT_OK;
}