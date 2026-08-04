/******************************************************************************
 * @file    packet_builder.h
 * @brief   TCP packet builder
 *
 * Copyright (c) 2026
 *
 * Packet Builder is responsible for constructing the packet header.
 *
 * Responsibilities:
 *      - Fill packet header
 *      - Update sequence number
 *
 * Packet Builder does NOT:
 *      - Allocate memory
 *      - Generate payload
 *      - Send packet
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_PACKET_BUILDER_H
#define TCP_DATA_CLIENT_PACKET_BUILDER_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"

/*=============================================================================
 * Packet Protocol Version
 *============================================================================*/

/*
 * Version Encoding
 *
 *      High Byte : Major
 *      Low Byte  : Minor
 *
 * Example:
 *
 *      0x0100 = Version 1.0
 *      0x0101 = Version 1.1
 *      0x0200 = Version 2.0
 */
#define TCP_PACKET_VERSION_MAJOR          (1U)
#define TCP_PACKET_VERSION_MINOR          (0U)

#define TCP_PACKET_VERSION               \
    ((uint16_t)((TCP_PACKET_VERSION_MAJOR << 8) | \
                 TCP_PACKET_VERSION_MINOR))

/*=============================================================================
 * Packet Flags
 *============================================================================*/

#define TCP_PACKET_FLAG_NONE              (0x00000000UL)

/*=============================================================================
 * Packet Header
 *============================================================================*/

TCP_PACKED_BEGIN

typedef struct TCP_PACKED
{
    uint16_t version;

    uint16_t header_size;

    uint64_t sequence;

    uint32_t payload_length;

    uint32_t flags;

    uint32_t reserved;

} tcp_packet_header_t;

TCP_PACKED_END

/*=============================================================================
 * Packet Constants
 *============================================================================*/

#define TCP_PACKET_HEADER_SIZE \
    ((uint32_t)sizeof(tcp_packet_header_t))

/*=============================================================================
 * Compile-Time Verification
 *============================================================================*/

TCP_STATIC_ASSERT(
    sizeof(tcp_packet_header_t) == 24,
    "Invalid packet header size");

/*=============================================================================
 * Interface
 *============================================================================*/

/**
 * @brief Initialize packet builder.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t packet_builder_initialize(void);

/**
 * @brief Deinitialize packet builder.
 *
 * @return TCP_RESULT_OK
 */
tcp_result_t packet_builder_deinitialize(void);

/**
 * @brief Build one packet header.
 *
 * Header is written into the specified packet buffer.
 *
 * Payload must already exist immediately after the header.
 *
 * Sequence number is automatically maintained by Packet Builder.
 *
 * @param[out] header
 *      Packet header.
 *
 * @param[in] payload_length
 *      Payload length in bytes.
 *
 * @return TCP_RESULT_OK if successful.
 */
tcp_result_t packet_builder_build(
    tcp_packet_header_t* header,
    uint32_t payload_length);

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_PACKET_BUILDER_H */