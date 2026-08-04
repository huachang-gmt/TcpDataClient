/******************************************************************************
 * @file    protocol.h
 * @brief   TcpDataClient network protocol definition
 *
 * Copyright (c) 2026
 *
 * This file defines the TcpDataClient network protocol shared by:
 *
 *      - Windows Simulator
 *      - STM32H7 TCP Client
 *      - Raspberry Pi TcpLogger
 *
 * This file contains protocol definitions only.
 *
 * It shall NOT:
 *
 *      - Allocate packet buffers
 *      - Generate packets
 *      - Parse packets
 *      - Send packets
 *
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_PROTOCOL_H
#define TCP_DATA_CLIENT_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 * Header Files
 *============================================================================*/

#include "common.h"
#include "packet_builder.h"
#include "config.h"

/*=============================================================================
 * Packet Length
 *============================================================================*/

/*
 * Total packet length.
 *
 * Header + Payload
 */
#define TCP_PACKET_LENGTH(payload_length) \
    (TCP_PACKET_HEADER_SIZE + (payload_length))

/*
 * Default packet length.
 */
#define TCP_DEFAULT_PACKET_LENGTH \
    TCP_PACKET_LENGTH(TCP_DEFAULT_PAYLOAD_LENGTH)

/*
 * Maximum packet length.
 */
#define TCP_MAX_PACKET_LENGTH \
    TCP_PACKET_LENGTH(TCP_MAX_PAYLOAD_LENGTH)

/*=============================================================================
 * Helper Macros
 *============================================================================*/

/*
 * Get payload pointer from a packet buffer.
 *
 * Buffer Layout:
 *
 * +------------------------+
 * | tcp_packet_header_t    |
 * +------------------------+
 * | Payload                |
 * +------------------------+
 */
#define TCP_PACKET_PAYLOAD(buffer) \
    ((uint8_t *)(buffer) + TCP_PACKET_HEADER_SIZE)

/*
 * Get packet header pointer from a packet buffer.
 */
#define TCP_PACKET_HEADER(buffer) \
    ((tcp_packet_header_t *)(buffer))

#ifdef __cplusplus
}
#endif

#endif /* TCP_DATA_CLIENT_PROTOCOL_H */