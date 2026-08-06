/******************************************************************************
 * @file    config.h
 * @brief   Compile-time configuration for TcpDataClient
 *
 * Copyright (c) 2026
 *
 * All values in this file are compile-time constants.
 * No runtime configuration should be placed here.
 ******************************************************************************/

#ifndef TCP_DATA_CLIENT_CONFIG_H
#define TCP_DATA_CLIENT_CONFIG_H

#include "common.h"

/*=============================================================================
 * Network Configuration
 *============================================================================*/

/* Default TCP Server Address */
//#define TCP_DEFAULT_SERVER_IP                "127.0.0.1"

/* 增加連接到 Raspberry Pi CM5 IP 位址 */
#define TCP_DEFAULT_SERVER_IP                "192.168.137.200"

/* Default TCP Server Port */
#define TCP_DEFAULT_SERVER_PORT              (7777U)

/*=============================================================================
 * Packet Configuration
 *============================================================================*/

/*
 * Payload Length
 *
 * This value represents application payload only.
 *
 * Packet Header will be added by packet_builder.
 *
 * Current Test Target:
 *
 *     Payload = 800 Bytes
 *
 */
#define TCP_DEFAULT_PAYLOAD_LENGTH            (800U)

/*
 * Supported Payload Length
 */
#define TCP_PAYLOAD_LENGTH_SMALL              (400U)

#define TCP_PAYLOAD_LENGTH_LARGE              (1024U)

#define TCP_MAX_PAYLOAD_LENGTH                (2048U)

/*=============================================================================
 * Transmission Configuration
 *============================================================================*/

/*
 * Send Interval
 *
 * Unit : millisecond
 */
#define TCP_DEFAULT_SEND_INTERVAL_MS          (1U)

/*
 * Number of packets to send.
 *
 * 0 = Infinite
 * 1000ULL 只傳 1000 個 packet      TCP_DEFAULT_PACKET_COUNT 
 */
#define TCP_DEFAULT_PACKET_COUNT              (TCP_PACKET_COUNT_INFINITE)

#define TCP_PACKET_COUNT_INFINITE             (0ULL)

/*=============================================================================
 * Reconnect Configuration
 *============================================================================*/

/*
 * Delay before reconnect.
 */
#define TCP_RECONNECT_INTERVAL_MS             (1000U)

/*
 * Connection timeout.
 */
#define TCP_CONNECT_TIMEOUT_MS                (3000U)

/*=============================================================================
 * Statistics Configuration
 *============================================================================*/

#define TCP_ENABLE_STATISTICS                 (1U)

#define TCP_STATISTICS_UPDATE_MS              (1000U)

/*=============================================================================
 * Console Configuration
 *============================================================================*/

#define TCP_ENABLE_CONSOLE                    (1U)

#define TCP_ENABLE_COLOR_OUTPUT               (0U)

/*=============================================================================
 * Logging Configuration
 *============================================================================*/

#define TCP_ENABLE_LOG_INFO                   (1U)

#define TCP_ENABLE_LOG_WARNING                (1U)

#define TCP_ENABLE_LOG_ERROR                  (1U)

#define TCP_ENABLE_LOG_DEBUG                  (0U)

/*=============================================================================
 * Build Configuration
 *============================================================================*/

/*
 * Enable automatic socket reconnect.
 */
#define TCP_ENABLE_AUTO_RECONNECT             (1U)

/*
 * Verify payload after generation.
 */
#define TCP_ENABLE_PAYLOAD_VERIFY             (0U)

/*
 * Enable packet CRC verification.
 *
 * Reserved for future protocol extension.
 */
#define TCP_ENABLE_PACKET_CRC                 (0U)

#endif /* TCP_DATA_CLIENT_CONFIG_H */