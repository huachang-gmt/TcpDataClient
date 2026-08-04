/******************************************************************************
 * @file    socket_win32.c
 * @brief   Windows socket implementation
 *
 * Copyright (c) 2026
 *
 * Platform Layer
 *
 * Responsibilities:
 *      - WSAStartup / WSACleanup
 *      - TCP connect
 *      - TCP disconnect
 *      - Reliable send
 *
 ******************************************************************************/

#include "socket_if.h"

#if TCP_PLATFORM_WINDOWS

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define SOCKET_INVALID_HANDLE ((SOCKET)INVALID_SOCKET)

/*=============================================================================
 * Public Functions
 *============================================================================*/

tcp_result_t socket_if_initialize(void)
{
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        return TCP_RESULT_ERROR;
    }

    return TCP_RESULT_OK;
}

tcp_result_t socket_if_deinitialize(void)
{
    WSACleanup();

    return TCP_RESULT_OK;
}

tcp_result_t socket_if_connect(
    socket_handle_t* handle,
    const socket_config_t* config)
{
    SOCKET sock;
    struct sockaddr_in addr;

    if ((handle == NULL) ||
        (config == NULL) ||
        (config->server_ip == NULL) ||
        (config->server_port == 0U))
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    handle->native_handle =
        SOCKET_HANDLE_INVALID;

    sock = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (sock == INVALID_SOCKET)
    {
        return TCP_RESULT_ERROR;
    }

    ZeroMemory(
        &addr,
        sizeof(addr));

    addr.sin_family = AF_INET;

    addr.sin_port =
        htons(config->server_port);

    if (inet_pton(
            AF_INET,
            config->server_ip,
            &addr.sin_addr) != 1)
    {
        closesocket(sock);

        return TCP_RESULT_ERROR;
    }

    if (connect(
            sock,
            (struct sockaddr*)&addr,
            sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);

        return TCP_RESULT_DISCONNECTED;
    }

    handle->native_handle =
        (void*)(uintptr_t)sock;

    return TCP_RESULT_OK;
}

tcp_result_t socket_if_disconnect(
    socket_handle_t* handle)
{
    SOCKET sock;

    if (handle == NULL)
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if (handle->native_handle ==
        SOCKET_HANDLE_INVALID)
    {
        return TCP_RESULT_OK;
    }

    sock =
        (SOCKET)(uintptr_t)
        handle->native_handle;

    closesocket(sock);

    handle->native_handle =
        SOCKET_HANDLE_INVALID;

    return TCP_RESULT_OK;
}

tcp_result_t socket_if_send(
    socket_handle_t handle,
    const uint8_t* buffer,
    uint32_t length)
{
    uint32_t total_sent = 0U;

    SOCKET sock;

    if (handle.native_handle ==
        SOCKET_HANDLE_INVALID)
    {
        return TCP_RESULT_DISCONNECTED;
    }

    if ((buffer == NULL) &&
        (length > 0U))
    {
        return TCP_RESULT_INVALID_PARAMETER;
    }

    if (length == 0U)
    {
        return TCP_RESULT_OK;
    }

    sock =
        (SOCKET)(uintptr_t)
        handle.native_handle;

    while (total_sent < length)
    {
        int remaining;
        int sent;

        remaining =
            (int)(length - total_sent);

        sent = send(
            sock,
            (const char*)
                (buffer + total_sent),
            remaining,
            0);

        if (sent == SOCKET_ERROR)
        {
            return TCP_RESULT_DISCONNECTED;
        }

        if (sent == 0)
        {
            return TCP_RESULT_DISCONNECTED;
        }

        total_sent +=
            (uint32_t)sent;
    }

    return TCP_RESULT_OK;
}

socket_state_t socket_if_get_state(
    socket_handle_t handle)
{
    if (handle.native_handle == SOCKET_HANDLE_INVALID)
    {
        return SOCKET_STATE_CLOSED;
    }

    return SOCKET_STATE_CONNECTED;
}

#endif /* TCP_PLATFORM_WINDOWS */