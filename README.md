


### 1. 我知道我們現在在哪裡

目前不是在設計 Logger，而是已經進入：

**Integration Phase → TcpDataClient 驗證 TcpLogger**

也就是：

```text
TcpDataClient.exe
        │
        │ TCP
        ▼
TcpLogger.exe
        │
        ▼
BufferPool
        │
        ▼
RingBuffer
        │
        ▼
DiskWriter
        │
        ▼
LOG_00000001.bin
```

Windows 的 `TcpDataClient` 是 **STM32H7 Simulator**，不是最終產品。

---

### 2. 我知道兩邊的語言與責任不能混在一起

```text
TcpLogger
    C++17
    Windows / Raspberry Pi CM5
```

而：

```text
TcpDataClient
    ANSI C
    Windows / Linux / STM32H7
```

所以 TcpDataClient 的程式碼必須一直維持：

* ANSI C
* No STL
* No dynamic memory
* Embedded coding style
* Platform abstraction
* 可直接朝 STM32H7 移植

Windows 的 `socket_win32.c` 只是平台實作。

未來：

```text
socket_linux.c
socket_lwip.c
```

上層 `tcp_client.c` 不應該知道自己現在跑在哪個 OS。

---

### 3. 我知道目前 Header 已經不是「可以隨便改」的狀態

目前已經有：

```text
common.h
config.h
protocol.h
packet_builder.h
packet_builder.c
socket_if.h
socket_win32.c
data_source.h
data_source_simulator.c
statistics.h
statistics.c
tcp_client.h
```

因此下一步我不會直接開始寫 `tcp_client.c`。

會先做你指定的：

> **重新檢查所有 Header 是否一致。**

而且檢查的不是只有「能不能 compile」。

我會特別檢查：

* API ownership
* object lifetime
* buffer lifetime
* const correctness
* sequence ownership
* packet ownership
* payload ownership
* zero-copy 是否真的成立
* socket send 的 lifetime
* partial send
* reconnect
* error handling
* statistics ownership
* thread safety
* 1 ms producer 的 scalability
* Windows / Linux / STM32 差異
* `uint32_t / uint16_t / uint8_t` 等 fixed-width types
* endian
* struct padding
* protocol layout
* future STM32 memory constraints

如果發現問題，我會**先提出來討論，而不是偷偷修改架構**。

---

### 4. 我知道 Payload 的責任

這點非常重要：

```text
Data Source
     │
     │ 800 Bytes
     ▼
Packet Builder
     │
     │ Header + Payload metadata
     ▼
TCP Client
     │
     ▼
Socket
```

**Packet Builder 不負責產生 Payload。**

Simulator 現在每 1 ms 提供：

```text
800 Bytes

00 01 02 03 ... FF 00 01 ...
```

目的是讓我們最後可以做：

```text
TcpDataClient
       ↓
TcpLogger
       ↓
LOG_00000001.bin
       ↓
Hex Compare
```

確認資料是否完全一致。

---

### 5. 我也知道真正的驗證不是「TCP 有連線」

這次 Integration Test 的真正成功條件是：

```text
TcpDataClient.exe
        │
        │ 800-byte payload / 1 ms
        ▼
TcpLogger.exe
        │
        ▼
TcpServer
        │
        ▼
BufferPool
        │
        ▼
RingBuffer
        │
        ▼
DiskWriter
        │
        ▼
LOG_00000001.bin
```

最後驗證：

**送出的 Payload 與 `.bin` 裡面的 Payload 是否 byte-for-byte 一致。**

也就是不只是：

> Connected / Received / File Created

而是要確認：

> **End-to-End Data Integrity**

---

### 6. 下一階段的實際順序我也清楚

你指定的是：

#### Step 0 — Header Consistency Review

先檢查：

```text
common.h
config.h
protocol.h
packet_builder.h
socket_if.h
data_source.h
statistics.h
tcp_client.h
```

以及它們彼此之間的 dependency / ownership。

↓

#### Step 1 — `tcp_client.c`

完成真正的：

```text
Data Source
    ↓
Packet Builder
    ↓
TCP Client
    ↓
Socket Abstraction
```

並處理產品級 TCP Client 必須面對的問題，而不是寫一個簡單的：

```c
connect();
send();
```

↓

#### Step 2 — `logger.h`

讓 `main.c` 可以以乾淨的 application layer 使用 Logger。

↓

#### Step 3 — `main.c`

負責：

* initialization
* application loop
* simulator / client 啟動
* statistics console display
* shutdown

而 **Statistics 本身不負責 Console output**，這個責任仍然在 `main.c`。

↓

#### Step 4 — Visual Studio Build

↓

#### Step 5 — 執行：

```text
TcpLogger.exe
TcpDataClient.exe
```

↓

#### Step 6 — 產生：

```text
LOG_00000001.bin
```

↓

#### Step 7 — Hex Compare

確認：

```text
Simulator Payload
        ==
LOG file payload
```

---

## 最重要的是：我不會把這次工作退回「教學範例」

我知道你現在要驗證的是**產品架構**，所以接下來我會特別避免幾種做法：
```
❌ 為了讓程式容易寫而把 layer 合併
❌ 在 `tcp_client.c` 裡直接操作 WinSock
❌ 把 Payload 複製來複製去
❌ 使用 `malloc()`
❌ 使用 STL
❌ 讓 Statistics 印 Console
❌ 讓 Packet Builder 自己生成 Payload
❌ 把 Windows API 滲透到上層
❌ 為了 Windows Simulator 而犧牲 STM32 portability
❌ 發現問題就直接重新設計整個 Architecture
```

而是維持我們之前確立的核心：

> **Single Responsibility + Zero Copy + Thread Safe + Platform Abstraction + No Dynamic Memory + Cross Platform + STM32-portable**

---

所以答案是：

**是，我已經知道上一階段做到哪裡，也知道這一階段真正要驗證什麼。**

而且我會把你這份提示詞視為目前這個新對話的 **Architecture Baseline**。

**下一步不應該直接寫 `tcp_client.c`。**

應該先做你要求的：

> **完整檢查目前所有 Header 的一致性與 Architecture / Memory / Lifetime / Race Condition / Scalability / Cross Platform / STM32 移植性。**

如果檢查沒有問題，我們再依序進入：

```text
Header Review
     ↓
tcp_client.c
     ↓
logger.h
     ↓
main.c
     ↓
Visual Studio Build
     ↓
TcpDataClient.exe
     ↓
TcpLogger.exe
     ↓
LOG_00000001.bin
     ↓
Hex Compare
     ↓
End-to-End PASS
```

這就是我現在掌握的完整接續狀態。



---

## 一、專案最終目標

這是一個跨平台 TCP 資料傳輸系統。

最終產品架構：

```text
STM32H755
    │
    │ TCP Client
    │ EtherCAT Data
    │ 800 bytes / 1 ms
    ▼
Raspberry Pi CM5
    │
    │ TCP Server
    ▼
TcpLogger
    │
    ▼
Binary Log Files
```

STM32H755 是實際產品端的 TCP Client。

Raspberry Pi CM5 是最終產品端的 TCP Server + Logger。

Windows PC 目前只是開發與驗證平台，不是最終產品。

---

# 二、非常重要的設計原則

## 1. Windows 只是 Simulator

目前在 Windows 11 上開發 TcpDataClient，是為了：

* 驗證 TCP Client
* 驗證 Packet Protocol
* 驗證 Data Source
* 驗證 sequence
* 驗證 1 ms transmission
* 驗證 TcpLogger Server
* 驗證資料完整性

但是：

> **絕對不可以為了 Windows 測試而設計出 STM32H755 無法使用的架構。**

所有核心模組都必須保持：

* C17
* platform independent
* no dynamic memory
* no C++
* no Windows-specific code in core modules
* no Linux-specific code in core modules
* no direct socket API in tcp_client.c
* socket platform-specific code 必須隔離
* data source 與 TCP 完全分離
* packet protocol 與 transport 完全分離
* future STM32H755 + LwIP 可以直接使用相同核心架構

Windows / Linux / STM32 差異只能存在 platform layer。

---

# 三、目前 TcpDataClient 已經完成的檔案

目前已完成：

```text
common.h
config.h
protocol.h
packet_builder.h
socket_if.h
data_source.h
statistics.h
tcp_client.h

socket_win32.c
statistics.c
packet_builder.c
data_source_simulator.c

logger.h
logger.c

tcp_client.c
main.c
```

目前 Windows 版本已經可以完整編譯與 Link。

---

# 四、目前 Packet Protocol

Packet Header 固定：

```text
24 bytes
```

Header：

```c
typedef struct
{
    uint16_t version;
    uint16_t header_size;
    uint64_t sequence;
    uint32_t payload_length;
    uint32_t flags;
    uint32_t reserved;

} tcp_packet_header_t;
```

Wire format 是：

> Little Endian

Header layout：

```text
Offset   Size    Field
0        2       Version
2        2       Header Size
4        8       Sequence
12       4       Payload Length
16       4       Flags
20       4       Reserved
```

Packet version：

```text
1.0
```

Encoded as：

```text
0x0100
```

目前：

```text
Payload = 800 bytes
Header  = 24 bytes
Total   = 824 bytes
```

所以目前 Windows Simulator 每個 TCP application packet 應該是：

```text
824 bytes
```

---

# 五、目前 Config

目前重要設定：

```text
TCP_DEFAULT_SERVER_IP       = "127.0.0.1"
TCP_DEFAULT_SERVER_PORT     = 7777

TCP_DEFAULT_PAYLOAD_LENGTH = 800

TCP_DEFAULT_SEND_INTERVAL_MS = 1

TCP_DEFAULT_PACKET_COUNT   = 1000

TCP_PACKET_COUNT_INFINITE  = 0

TCP_MAX_PAYLOAD_LENGTH     = 2048

TCP_RECONNECT_INTERVAL_MS  = 1000

TCP_CONNECT_TIMEOUT_MS     = 3000
```

Statistics：

```text
enabled
update interval = 1000 ms
```

Auto reconnect：

```text
enabled
```

---

# 六、Data Source

目前 simulator：

```text
data_source_simulator.c
```

Simulator 產生 deterministic test pattern。

Payload：

```text
payload[i] =
    (packet_index + i) & 0xFF
```

例如：

Packet 0：

```text
00 01 02 03 04 ...
```

Packet 1：

```text
01 02 03 04 05 ...
```

Packet 2：

```text
02 03 04 05 06 ...
```

因此 TcpLogger 未來可以很容易驗證：

* packet sequence
* payload pattern
* packet loss
* packet order
* corruption

Data Source API：

```c
tcp_result_t data_source_initialize(
    const data_source_config_t* config);

tcp_result_t data_source_deinitialize(void);

tcp_result_t data_source_read(
    uint8_t* payload,
    uint32_t payload_capacity,
    uint32_t* payload_length);
```

Data Source 不知道：

* TCP
* Socket
* Header
* Sequence

未來 STM32H755 可以替換成：

```text
DATA_SOURCE_ETHERCAT
```

而不用修改 TcpClient 核心架構。

---

# 七、Packet Builder

目前：

```text
packet_builder.c
```

負責：

* 建立 header
* sequence number
* Little Endian encoding

Sequence：

```text
0
1
2
3
...
```

每建立一個 packet：

```text
sequence++
```

目前 sequence 是：

```c
static uint64_t s_sequence;
```

初始化時：

```text
0
```

---

# 八、Socket abstraction

目前：

```text
socket_if.h
```

平台介面：

```c
tcp_result_t socket_if_initialize(void);

tcp_result_t socket_if_deinitialize(void);

tcp_result_t socket_if_connect(
    socket_handle_t* handle,
    const socket_config_t* config);

tcp_result_t socket_if_disconnect(
    socket_handle_t* handle);

tcp_result_t socket_if_send(
    socket_handle_t handle,
    const uint8_t* buffer,
    uint32_t length);

socket_state_t socket_if_get_state(
    socket_handle_t handle);
```

重要設計：

```text
tcp_client.c
      │
      ▼
socket_if.h
      │
      ├── socket_win32.c
      ├── socket_linux.c
      └── socket_lwip.c
```

tcp_client.c 不可以直接：

```text
send()
connect()
closesocket()
socket()
```

也不能直接使用 Windows socket API。

---

# 九、Windows Socket Layer

目前：

```text
socket_win32.c
```

已經完成：

* WSAStartup
* socket
* connect
* disconnect
* reliable send
* socket state

`socket_if_send()` 目前會處理 partial send：

```text
send()
send()
send()
...
直到完整 buffer 傳送完成
```

而且不修改 source buffer。

目前 Windows socket layer 可以正常編譯。

---

# 十、Statistics

目前：

```text
statistics.c
statistics.h
```

統計：

```text
packet_count
total_bytes
total_payload_bytes
elapsed_time_ms
packets_per_second
bytes_per_second
```

Windows：

```text
GetTickCount64()
```

Linux：

```text
clock_gettime(CLOCK_MONOTONIC)
```

STM32：

```text
HAL_GetTick()
```

這是 platform-specific time implementation，但 statistics API 保持跨平台。

---

# 十一、Logger

目前：

```text
logger.h
logger.c
```

Logger 只負責：

* info
* warning
* error
* debug

Logger 不負責：

* TCP
* Socket
* Packet
* Payload
* Statistics

---

# 十二、目前 Windows Build 狀態

已經成功使用：

```bat
cl /nologo /W4 /std:c17 /Iinclude ^
src\main.c ^
src\tcp_client.c ^
src\packet_builder.c ^
src\data_source_simulator.c ^
src\statistics.c ^
src\socket_win32.c ^
src\logger.c ^
/Fe:TcpDataClient.exe ^
/link Ws2_32.lib
```

結果：

```text
main.c
tcp_client.c
packet_builder.c
data_source_simulator.c
statistics.c
socket_win32.c
logger.c
正在產生程式碼...
```

沒有：

```text
warning
error
```

因此：

> Windows TcpDataClient 已經通過 `/W4` 全量編譯與 Link。

---

# 十三、明天第一件事情：不要先改程式

明天首先要進行：

# 第一階段：Windows 本機 TCP Client ↔ TCP Server

目前目標：

```text
Windows TcpDataClient
        │
        │ TCP
        │ 127.0.0.1:7777
        ▼
Windows TcpLogger
```

這是第一個真正的 integration test。

必須先確認：

1. TcpLogger 啟動
2. TcpLogger listen `127.0.0.1:7777`
3. TcpDataClient connect
4. TCP connection 成功
5. Client 傳送 packet
6. Server 收到完整 packet
7. Header = 24 bytes
8. Payload = 800 bytes
9. Total = 824 bytes
10. sequence 從 0 開始
11. sequence 連續遞增
12. payload_length = 800
13. payload pattern 正確
14. 1000 packets 可以完整傳輸
15. 沒有 packet loss
16. 沒有 packet corruption
17. TcpLogger 正確寫檔
18. 最終確認 binary file size
19. 驗證檔案內容

---

# 十四、特別注意 TCP Stream

明天測試 TcpLogger 時，必須記住：

> TCP 沒有 packet boundary。

TcpClient 呼叫：

```text
send(824 bytes)
```

不代表 Server 一定：

```text
recv() == 824
```

Server 可能收到：

```text
300
524
```

或者：

```text
824
```

或者：

```text
100
200
524
```

甚至多個 application packet 一次收到：

```text
1648
```

所以 TcpLogger Server 必須按照：

```text
24-byte header
        ↓
讀取 payload_length
        ↓
等待完整 payload
        ↓
組成完整 application packet
```

來解析。

不能用：

```text
一次 recv() = 一個 packet
```

這個錯誤假設。

---

# 十五、第二階段

Windows 本機測試成功後：

```text
Windows TcpDataClient
        ↓
Raspberry Pi CM5 TcpLogger
```

此時：

Windows：

```text
TCP Client
```

Raspberry Pi CM5：

```text
TCP Server
```

目的：

驗證：

* Windows → Ethernet → CM5
* TCP connection
* packet protocol
* sequence
* payload
* file logging

這一步會正式把 TcpLogger 從 Windows Server 移植到：

> Raspberry Pi CM5 / Linux

---

# 十六、第三階段

再進行反方向驗證：

```text
STM32H755
    │
    │ TCP Client
    ▼
Windows TcpLogger
    │
    ▼
Windows binary log
```

此時 STM32H755 是真正 Client。

Data Source 從：

```text
DATA_SOURCE_SIMULATOR
```

替換成：

```text
DATA_SOURCE_ETHERCAT
```

核心 TcpClient 不應該因為 Data Source 改變而重新設計。

---

# 十七、最終階段

最後才進行：

```text
STM32H755
    │
    │ TCP Client
    │ 800 bytes / 1 ms
    ▼
Raspberry Pi CM5
    │
    │ TCP Server
    ▼
TcpLogger
    │
    ▼
Binary Files
```

這才是最終產品架構。

---

# 十八、STM32H755 的特殊要求

STM32H755 實際產品端非常重要。

請始終考慮：

* RAM 有限
* Flash 有限
* LwIP
* `tcp_pcb`
* Ethernet
* EtherCAT data
* 1 ms transmission
* 不可以 dynamic allocation
* 不可以 Windows API
* 不可以 Linux API
* 不可以依賴 thread 才能工作
* 核心 API 必須可以在 bare-metal / RTOS environment 使用
* TcpClient 必須能被 main loop 或 timer-driven architecture 呼叫
* socket layer 必須能替換成 LwIP implementation

Windows 測試成功只是驗證工具。

不是最終 architecture。

---

# 十九、目前最重要的測試資料

目前：

```text
Payload:
800 bytes

Header:
24 bytes

Application Packet:
824 bytes

Interval:
1 ms

Packet count:
1000
```

所以正常情況：

```text
1000 packets
×
824 bytes
=
824,000 bytes
```

Payload：

```text
1000
×
800
=
800,000 bytes
```

Header：

```text
1000
×
24
=
24,000 bytes
```

總 TCP application data：

```text
824,000 bytes
```

注意：

這只是 application data。

TCP/IP/Ethernet headers 不包含在這個數字裡。

---

# 二十、明天的工作順序

請嚴格按照以下順序，不要跳步：

### Step 1

確認：

```text
TcpDataClient.exe
```

存在。

### Step 2

確認 TcpLogger Server 目前 Windows 版本可以：

```text
listen 127.0.0.1:7777
```

### Step 3

啟動 TcpLogger。

### Step 4

啟動 TcpDataClient。

### Step 5

觀察：

```text
connect
packet transmission
statistics
disconnect
```

### Step 6

檢查 TcpLogger：

```text
received bytes
packet count
sequence
payload length
file size
```

### Step 7

驗證 binary file。

### Step 8

如果發現錯誤：

先判斷屬於：

```text
Client
Protocol
Socket
TCP stream parsing
Server
DiskWriter
```

再針對責任模組修改。

不要沒有證據就修改 tcp_client.c。

---

# 二十一、非常重要

目前 TcpDataClient 的核心程式已經經過多次檢查與修改。

所以明天：

> 不要因為看到可以「再優化」就任意修改已經通過 `/W4` 的核心模組。

先測試。

由實際結果決定下一步。

尤其是：

```text
tcp_client.c
packet_builder.c
data_source_simulator.c
socket_win32.c
statistics.c
```

目前都已經通過 Windows `/W4` 全量編譯。

---

# 二十二、你的角色

請以：

> 「產品級 Embedded TCP / STM32H755 / LwIP / Raspberry Pi CM5 / Linux」

的角度協助我。

不是單純寫一個 Windows TCP demo。

所有決策都必須考慮：

```text
Windows Simulator
        ↓
Linux CM5
        ↓
STM32H755
```

三者共用 protocol 與 core architecture。

---

# 明天開始時

我會先告訴你目前 TcpLogger 的狀態，然後開始：

> **Windows 本機 TcpDataClient ↔ TcpLogger 實際資料傳輸測試**

請從這裡繼續，不要重新開始設計專案。



---


# 三階段測試路線
我完全理解，而且我會把**今天的三階段測試路線固定下來**。今天不是繼續「開發更多功能」，而是開始做真正的 Integration Test。

你昨天完成的是 **TcpDataClient 的核心骨架與 Windows 編譯驗證**；今天開始，要用實際資料流把 `TcpDataClient → TcpLogger → Binary File` 串起來。

---

# 一、我先確認我們現在的最終架構

今天開始，我們的驗證路線是：

```text
                    第一階段
              Windows 本機 Integration Test
              
   ┌───────────────────┐
   │ TcpDataClient.exe │
   │   TCP Client      │
   └────────┬──────────┘
            │
            │ TCP
            │ 127.0.0.1:7777
            ▼
   ┌──────────────────┐
   │    TcpLogger     │
   │   TCP Server     │
   │   Disk Writer    │
   └────────┬─────────┘
            │
            ▼
       Binary Log File
```

第一階段**完全不碰 STM32，也不碰 CM5**。

目的只有一個：

> **證明我們昨天做好的 TcpDataClient 與 TcpLogger Server，在同一台 Windows 電腦上可以正確完成「連線 → 傳輸 → TCP Stream 重組 → Packet 解碼 → 建檔」。**

---

# 二、第二階段

第一階段成功之後：

```text
                Windows PC
        ┌─────────────────────┐
        │ TcpDataClient.exe   │
        │ TCP Client          │
        └─────────┬───────────┘
                  │
                  │ Ethernet / TCP
                  │
                  ▼
        ┌─────────────────────┐
        │ Raspberry Pi CM5    │
        │                     │
        │ TcpLogger           │
        │ TCP Server          │
        │ Disk Writer         │
        └─────────┬───────────┘
                  │
                  ▼
             Binary Files
                  │
                  ▼
          FTP Server Root
```

這裡有一個很重要的事情：

### TcpLogger 的 Binary Log File

最後要放在：

> **Raspberry Pi CM5 FTP Server 的根目錄**

這樣第二階段不只是驗證 TCP。

還會同時驗證：

```text
Windows Client
      ↓
Ethernet
      ↓
CM5 TCP Server
      ↓
Logger
      ↓
Disk
      ↓
FTP Root
      ↓
Windows FileZilla
```

也就是完整的資料鏈。

---

# 三、第三階段才是真正產品方向

最後：

```text
                  EtherCAT
                     │
                     ▼
              ┌─────────────┐
              │ STM32H755   │
              │             │
              │ Data Source │
              │     ↓       │
              │ TcpClient   │
              │     ↓       │
              │ LwIP        │
              └──────┬──────┘
                     │
                     │ TCP
                     │ 800 bytes / 1 ms
                     ▼
              ┌─────────────┐
              │ Raspberry   │
              │ Pi CM5      │
              │             │
              │ TcpLogger   │
              │ TCP Server  │
              │ DiskWriter  │
              └──────┬──────┘
                     │
                     ▼
               Binary Files
                     │
                     ▼
                  FTP Root
```

這才是：

> **最終產品架構。**

---

# 四、今天最重要的原則

今天我們**不要先修改程式**。

昨天：

```text
TcpDataClient
        ↓
/W4
        ↓
無 warning
無 error
        ↓
成功 Link
        ↓
TcpDataClient.exe
```

這代表「編譯階段」已經通過。

今天要驗證的是：

```text
Compile Test
     ↓
Integration Test
     ↓
Functional Test
     ↓
Data Integrity Test
```

所以現在不要看到程式就想：

> 「這裡是不是可以再優化？」

先不要。

如果測試失敗，我們再根據**實際證據**判斷是哪一層有問題。

---

# 五、第一階段：Windows 本機測試

我們今天先只做：

```text
TcpDataClient.exe
        ↓
127.0.0.1:7777
        ↓
TcpLogger
        ↓
Binary File
```

而且我會採取：

> **一步、確認、再下一步**

而不是一次叫你執行十幾個指令。

---

# Step 1：先確認 TcpDataClient.exe

請先打開：

```text
Developer Command Prompt for Visual Studio 2026
```

或者你昨天編譯 TcpDataClient 的命令提示字元。

進入 TcpDataClient 專案目錄。

先執行：

```bat
dir TcpDataClient.exe
```

應該看到類似：

```text
TcpDataClient.exe
```

然後：

```bat
TcpDataClient.exe
```

**先不要急著讓它長時間執行。**

我們第一個目標只是確認：

```text
TcpDataClient.exe
```

可以正常啟動。

---

# Step 2：確認 TcpLogger Server

這一步非常重要。

我們要確認昨天已經完成的 Windows TcpLogger Server 是不是目前可以直接：

```text
127.0.0.1
Port 7777
```

進行 listen。

也就是：

```text
TcpLogger
    │
    └── listen()
          │
          └── 127.0.0.1:7777
```

**這裡我暫時不要求你修改 TcpLogger。**

先啟動你目前已經完成的 TcpLogger Server。

正常應該看到類似：

```text
TcpLogger starting...
Server listening...
127.0.0.1:7777
Waiting for client...
```

實際輸出文字可能跟這個不同，沒關係。

---

# Step 3：確認 Windows Port 7777

這一步我希望我們不要只相信畫面。

另外開一個 CMD：

```bat
netstat -ano | findstr :7777
```

如果 TcpLogger 正在 listen，應該看到類似：

```text
TCP    127.0.0.1:7777    0.0.0.0:0    LISTENING    xxxx
```

其中：

```text
7777
```

就是我們目前的 TCP Server Port。

這一步確認的是：

> **Windows OS 層面真的有人在 Port 7777 listen。**

---

# Step 4：啟動 TcpDataClient

Server 確定：

```text
127.0.0.1:7777
LISTENING
```

之後，再啟動：

```bat
TcpDataClient.exe
```

目前 Config 是：

```text
Server IP:
127.0.0.1

Server Port:
7777

Payload:
800 bytes

Interval:
1 ms
```

因此 TcpDataClient 應該：

```text
Initialize
    ↓
Connect
    ↓
Connected
    ↓
Data Source
    ↓
Packet Builder
    ↓
Socket
    ↓
TCP
```

---

# Step 5：第一個重要觀察點——TCP Connection

此時我們**先不看檔案**。

只看：

```text
Client
   │
   │ connect()
   ▼
Server
   │
   │ accept()
   ▼
CONNECTED
```

TcpDataClient 如果顯示類似：

```text
Connected to 127.0.0.1:7777
```

就代表：

### 第一個測試 PASS

```text
TCP Connection = PASS
```

如果這裡失敗：

> 不要繼續往下測。

我們直接處理 Connection 問題。

---

# 六、Step 6：開始傳送 1000 packets

目前設定：

```text
Packet Count = 1000
```

每 packet：

```text
Header      24 bytes
Payload    800 bytes
--------------------
Total      824 bytes
```

所以：

```text
824 × 1000
=
824,000 bytes
```

TcpDataClient 應該傳送：

```text
Sequence 0
Sequence 1
Sequence 2
...
Sequence 999
```

Payload：

```text
Packet 0:
00 01 02 03 04 ...

Packet 1:
01 02 03 04 05 ...

Packet 2:
02 03 04 05 06 ...
```

---

# 七、Step 7：TcpLogger Server 必須驗證 TCP Stream

這是今天**最重要的測試之一**。

假設 Client：

```c
send(buffer, 824)
```

Server 絕對不能假設：

```text
recv() == 824
```

例如 Server 可能收到：

```text
recv()
300 bytes

recv()
524 bytes
```

這兩次才組成：

```text
824 bytes
```

也可能：

```text
recv()
1648 bytes
```

代表：

```text
Packet 0 = 824
Packet 1 = 824
```

所以 TcpLogger 必須按照：

```text
TCP byte stream
       │
       ▼
24-byte Header
       │
       ▼
payload_length = 800
       │
       ▼
等待 800 bytes
       │
       ▼
完整 Packet
```

解析。

這是我們今天真正要驗證的核心。

---

# 八、Step 8：Server 驗證第一個 Packet

第一個完整 packet 必須是：

```text
Header
├── version       = 0x0100
├── header_size   = 24
├── sequence      = 0
├── payload_length= 800
├── flags
└── reserved

Payload
└── 800 bytes
```

所以第一個 packet：

```text
Total = 824 bytes
```

而 Payload：

```text
00 01 02 03 04 05 06 ...
```

---

# 九、Step 9：驗證 Sequence

接下來 Server 必須看到：

```text
0
1
2
3
4
5
...
999
```

而不能：

```text
0
1
3
4
```

也不能：

```text
0
1
2
2
3
```

也不能：

```text
100
101
102
```

所以：

```text
Sequence continuity
```

是今天的核心測試項目。

---

# 十、Step 10：驗證 Payload

我們有一個非常好的地方：

> Simulator 不是亂數。

而是 deterministic pattern。

所以：

```text
payload[i] =
    (packet_index + i) & 0xFF
```

例如：

### Sequence 0

```text
00 01 02 03 04 05 ...
```

### Sequence 1

```text
01 02 03 04 05 06 ...
```

### Sequence 2

```text
02 03 04 05 06 07 ...
```

因此 Server 不只是：

```text
「收到 800 bytes」
```

而是可以真正驗證：

> **收到的 800 bytes 是不是正確的 800 bytes。**

這可以抓出：

```text
offset error
buffer corruption
packet misalignment
stream parsing error
sequence mismatch
```

---

# 十一、Step 11：驗證 1000 packets

最後：

```text
Expected packets:
1000
```

Expected payload:

```text
800,000 bytes
```

Expected header:

```text
24,000 bytes
```

Expected application data:

```text
824,000 bytes
```

所以我們要得到：

```text
Packets       = 1000
Payload       = 800,000
Header        = 24,000
Application   = 824,000
```

---

# 十二、Step 12：驗證 Binary File

這一步非常重要。

因為 TCP 收到正確，不代表 DiskWriter 一定正確。

所以要分開驗證：

```text
TCP Receive
     ↓
Packet Parser
     ↓
DiskWriter
     ↓
Binary File
```

如果目前 TcpLogger 的設計是：

> Binary file 只寫 Payload

那麼預期：

```text
1000 × 800
=
800,000 bytes
```

如果目前 DiskWriter 是：

> Header + Payload 全部寫入

那麼預期：

```text
1000 × 824
=
824,000 bytes
```

**這裡我不會自行猜測。**

我們會按照你目前 TcpLogger 的 `DiskWriter` 實作確認到底是哪一種。

---

# 十三、Step 13：最後做 Binary File Integrity Test

不是只看：

```text
檔案大小正確
```

而是要真正驗證內容。

理想結果：

```text
File
 │
 ├── Packet 0
 │     └── Payload 00 01 02...
 │
 ├── Packet 1
 │     └── Payload 01 02 03...
 │
 ├── Packet 2
 │     └── Payload 02 03 04...
 │
 ...
 │
 └── Packet 999
```

如此第一階段才算真正完成。

---

# 十四、第一階段 PASS 條件

我們會建立這張驗收表：

| 測試項目              | 必須結果 |
| ----------------- | ---- |
| TcpLogger 啟動      | PASS |
| Port 7777 LISTEN  | PASS |
| TcpDataClient 啟動  | PASS |
| TCP Connect       | PASS |
| Packet 0          | PASS |
| Header 24 bytes   | PASS |
| Payload 800 bytes | PASS |
| Total 824 bytes   | PASS |
| Sequence 0        | PASS |
| Sequence 0→999    | PASS |
| Payload pattern   | PASS |
| 1000 packets      | PASS |
| 0 packet loss     | PASS |
| 0 corruption      | PASS |
| DiskWriter        | PASS |
| Binary file       | PASS |
| File size         | PASS |
| File content      | PASS |

**全部 PASS，第一階段才算完成。**

---

# 十五、然後才進入第二階段：CM5

Windows 本機完成之後，我們才把：

```text
TcpLogger
```

從：

```text
Windows
```

移植到：

```text
Raspberry Pi CM5
```

此時：

```text
Windows
TcpDataClient
     │
     │ TCP
     ▼
Raspberry Pi CM5
TcpLogger
     │
     ▼
FTP Root
```

而且：

> **TcpDataClient 不應該因為 Server 從 Windows 變成 Linux 而修改核心程式。**

這正是我們現在 architecture 要證明的事情。

---

# 十六、CM5 階段我們還會多驗證一件事

檔案建立後：

```text
CM5
  │
  ├── TcpLogger
  │
  ├── Binary files
  │
  └── FTP Server
```

從 Windows 使用 FileZilla：

```text
Windows FileZilla
       │
       ▼
Raspberry Pi CM5
       │
       ▼
FTP Root
       │
       ▼
Binary Log File
```

確認：

```text
檔案存在
檔案大小正確
可以下載
下載後內容正確
```

這樣第二階段才 PASS。

---

# 十七、最後才碰 STM32H755

第三階段才把：

```text
Windows TcpDataClient
```

換成：

```text
STM32H755 TcpClient
```

但是：

```text
Packet Protocol
       │
       ├── 24-byte Header
       ├── Sequence
       ├── Payload Length
       └── 800-byte Payload
```

以及：

```text
Data Source
Packet Builder
TcpClient
Socket abstraction
```

這些核心概念都不能重新設計。

只需要把：

```text
DATA_SOURCE_SIMULATOR
```

換成：

```text
EtherCAT Data Source
```

以及：

```text
socket_win32.c
```

換成：

```text
socket_lwip.c
```

最後變成：

```text
STM32H755
   │
   ├── EtherCAT Data
   │
   ▼
Data Source
   │
   ▼
Packet Builder
   │
   ▼
TcpClient
   │
   ▼
socket_lwip.c
   │
   ▼
LwIP
   │
   ▼
Ethernet
   │
   ▼
CM5 TcpLogger
```

這才是我們從一開始設計 abstraction 的真正目的。

---

# 十八、所以今天我們不要一次做完三階段

今天我會陪你**一步一步實際跑**。

我們現在正式進入：

## 🟢 Phase 1 — Windows Local Integration Test

第一個動作非常簡單。

### 請你現在先做：

在 Windows 的 TcpDataClient 專案目錄執行：

```bat
dir TcpDataClient.exe
```

然後把**完整輸出結果貼給我**。

如果確認 `TcpDataClient.exe` 存在，我們下一步才啟動 TcpLogger。

**先不要修改任何 `.c/.h`。**

我們會按照：

```text
Step 1
  ↓
確認結果
  ↓
Step 2
  ↓
確認結果
  ↓
Step 3
  ↓
確認結果
  ↓
...
```

一路把第一階段真正跑通。

---

## 編譯命令

1. 使用 C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Visual Studio 2026\Visual Studio Tools\VC 資料夾下的 `x64 Native Tools Command Prompt for VS` 。 執行此，會跳出一個 Terminal 終端機視窗，這個視窗才有編譯命令，使用一般 Power shell 沒有。
2. 路徑 ： 
```bash
C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools>d:

D:\>cd D:\RaspberryPi\Raspberry_Pi_CM5\TcpDataClient

D:\RaspberryPi\Raspberry_Pi_CM5\TcpDataClient>
```
3. 編譯命令： 
```bash
cl /nologo /W4 /std:c17 /Iinclude /c src\tcp_client.c src\packet_builder.c src\data_source_simulator.c src\statistics.c src\socket_win32.c
```
4. 編譯結果：
```bash
D:\RaspberryPi\Raspberry_Pi_CM5\TcpDataClient>cl /nologo /W4 /std:c17 /Iinclude /c src\tcp_client.c src\packet_builder.c src\data_source_simulator.c src\statistics.c src\socket_win32.c
tcp_client.c
packet_builder.c
data_source_simulator.c
statistics.c
socket_win32.c
正在產生程式碼...
```
5. 單一檔案編譯命令
```bash
cl /nologo /W4 /std:c17 /Iinclude /c src\logger.c
cl /nologo /W4 /std:c17 /Iinclude /c src\main.c
```
6. 全部檔案編譯命令
```bat
cl /nologo /W4 /std:c17 /Iinclude ^
src\main.c ^
src\tcp_client.c ^
src\packet_builder.c ^
src\data_source_simulator.c ^
src\statistics.c ^
src\socket_win32.c ^
src\logger.c ^
/Fe:TcpDataClient.exe ^
/link Ws2_32.lib
```

---

# 一、現在完整的四階段測試路線

我們現在確定是：

```text
Phase 1
Windows TcpDataClient
        ↓
Windows TcpLogger
```

用途：

> 驗證我們自己寫的 Windows TCP Client 與 Windows TCP Server，以及完整 Packet Protocol、TCP Stream Parser、Logger、Binary File。

---

```text
Phase 2
STM32H7 TcpClient
        ↓
Windows TcpLogger
```

用途：

> 驗證真正的 STM32H7 TCP Client 能不能按照我們定義的 Protocol，正確把資料送到電腦。

這一步非常重要。

因為如果 Phase 2 成功，我們就知道：

```text
STM32H7
   ↓
LwIP
   ↓
TCP
   ↓
Windows TcpLogger
```

這條鏈是通的。

---

```text
Phase 3
Windows TcpDataClient
        ↓
Raspberry Pi CM5 TcpLogger
        ↓
Binary File
        ↓
FTP Root
```

用途：

> 驗證 TcpLogger 從 Windows 移植到 CM5/Linux，以及 CM5 建檔與 FTP Root 整合。

---

```text
Phase 4
STM32H7 TcpClient
        ↓
Raspberry Pi CM5 TcpLogger
        ↓
Binary File
        ↓
FTP Root
```

這才是：

# 最終產品測試

```text
STM32H7
   │
   │ EtherCAT Data
   │
   │ 800 bytes / 1 ms
   ▼
TCP Client
   │
   │ Ethernet / TCP
   ▼
Raspberry Pi CM5
   │
   ▼
TcpLogger
   │
   ▼
Binary Log
   │
   ▼
FTP Root
```

所以你這次補充後，整個開發路線就完整了。

---

# 二、你目前貼出的 TcpDataClient 狀態

你現在的：

```text
D:\RaspberryPi\Raspberry_Pi_CM5\TcpDataClient
```

已經有：

```text
TcpDataClient.exe
```

大小：

```text
146,944 bytes
```

而且 `.obj` 也都存在：

```text
main.obj
tcp_client.obj
packet_builder.obj
data_source_simulator.obj
statistics.obj
socket_win32.obj
logger.obj
```

這代表我們昨天確認的：

```text
C17
/W4
Compile
Link
```

結果仍然存在。

所以：

> **現在不要重新編譯，也不要修改 TcpDataClient。**

我們直接拿這個 `TcpDataClient.exe` 做 Phase 1。

---

# 三、今天第一個測試：Phase 1

我們現在先做：

```text
┌──────────────────────────┐
│ Windows PC               │
│                          │
│ TcpDataClient.exe        │
│ TCP Client               │
│                          │
│ 127.0.0.1:7777           │
│          │               │
│          │ TCP           │
│          ▼               │
│ TcpLogger                │
│ TCP Server               │
│                          │
│ DiskWriter               │
└──────────────────────────┘
```

這一階段**完全不碰 STM32**。

STM32 → Windows 是我們接下來的 Phase 2。

---

# 四、現在不要直接執行 TcpDataClient

這點我要特別提醒你。

你現在雖然已經有：

```text
TcpDataClient.exe
```

但它是：

> **TCP Client**

所以如果現在直接執行：

```bat
TcpDataClient.exe
```

而 Windows 的 TcpLogger Server 還沒有啟動，它應該會：

```text
connect
   ↓
connection failed
   ↓
retry
```

因此正確順序一定是：

```text
① 啟動 TcpLogger Server
       ↓
② 確認 7777 LISTEN
       ↓
③ 啟動 TcpDataClient
       ↓
④ Client connect
       ↓
⑤ 開始傳送
```

---

# 五、Step 1：先找到目前的 TcpLogger

現在我需要你先告訴我：

> **你目前 Windows 版 TcpLogger 放在哪裡，以及目前有沒有 `TcpLogger.exe`。**

因為你這次只貼了：

```text
D:\RaspberryPi\Raspberry_Pi_CM5\TcpDataClient
```

我不希望猜測你的 TcpLogger 路徑。

請先在 CMD 執行：

```bat
where /r D:\RaspberryPi\Raspberry_Pi_CM5 TcpLogger.exe
```

這個指令會從：

```text
D:\RaspberryPi\Raspberry_Pi_CM5
```

底下搜尋：

```text
TcpLogger.exe
```

---

# 六、如果找到 TcpLogger.exe

例如可能得到：

```text
D:\RaspberryPi\Raspberry_Pi_CM5\TcpLogger\build\Debug\TcpLogger.exe
```

或者：

```text
D:\RaspberryPi\Raspberry_Pi_CM5\TcpLogger\build\TcpLogger.exe
```

**先不要執行。**

把完整結果貼給我。

我會根據你目前實際的 TcpLogger 目錄結構，帶你進行：

```text
Step 1
TcpLogger 啟動
       ↓
Step 2
確認 127.0.0.1:7777
       ↓
Step 3
TcpDataClient 啟動
       ↓
Step 4
確認 TCP connection
       ↓
Step 5
確認第一個 packet
       ↓
Step 6
確認 1000 packets
       ↓
Step 7
確認 Binary File
```

---

# 七、另外，我已經把 STM32H7 測試正式加入基準

後面我們會另外建立這個測試：

```text
Phase 2
```

### STM32H7

```text
STM32H7
   │
   │ tcp_client.c
   │
   │ socket_lwip.c
   ▼
LwIP
   │
   ▼
Ethernet
   │
   ▼
Windows
   │
   ▼
TcpLogger
```

這一步尤其重要，因為它會驗證：

```text
STM32H7 Packet Builder
STM32H7 TcpClient
STM32H7 LwIP Socket Layer
        ↓
Windows TcpLogger
```

而且我們可以直接拿 Phase 1 的結果當作基準。

也就是：

```text
Phase 1
Windows Client → Windows Server
       ↓
Protocol 基準確認
```

然後：

```text
Phase 2
STM32 Client → Windows Server
       ↓
驗證 STM32 是否完全遵循相同 Protocol
```

這樣除錯會非常乾淨。

---

# 八、因此現在的順序不要搞錯

我們今天不是：

```text
Windows Client
→
CM5
→
STM32
```

而是：

```text
                ┌─────────────────────────┐
                │ Phase 1                 │
                │ Windows Client          │
                │       ↓                 │
                │ Windows Server          │
                └──────────┬──────────────┘
                           │
                           ▼
                ┌─────────────────────────┐
                │ Phase 2                 │
                │ STM32 Client            │
                │       ↓                 │
                │ Windows Server          │
                └──────────┬──────────────┘
                           │
                           ▼
                ┌─────────────────────────┐
                │ Phase 3                 │
                │ Windows Client          │
                │       ↓                 │
                │ CM5 Server              │
                └──────────┬──────────────┘
                           │
                           ▼
                ┌─────────────────────────┐
                │ Phase 4                 │
                │ STM32 Client            │
                │       ↓                 │
                │ CM5 Server              │
                │       ↓                 │
                │ FTP Root                │
                └─────────────────────────┘
```

這個順序非常合理。

---

## 現在先做一件事

在 CMD 執行：

```bat
where /r D:\RaspberryPi\Raspberry_Pi_CM5 TcpLogger.exe
```

**把完整結果貼給我。**

我拿到 TcpLogger.exe 的實際位置後，就從 **Phase 1 / Step 1「啟動 Windows TcpLogger」**開始，一步一步帶你測，不會一次丟一大堆操作給你。


```text
TcpDataClient/
├── .gitignore
├── README.md
├── include/
│   ├── common.h
│   ├── config.h
│   ├── protocol.h
│   ├── packet_builder.h
│   ├── socket_if.h
│   ├── data_source.h
│   ├── statistics.h
│   └── tcp_client.h
│
└── src/
    ├── main.c
    ├── tcp_client.c
    ├── packet_builder.c
    ├── data_source_simulator.c
    ├── statistics.c
    ├── socket_win32.c
    └── logger.c
```

## 第一階段目標
### tcp client 與 tcp server 都在同一部電腦內，每 256MByte 形成一個檔案

```text
Windows TcpDataClient
        │
        │ 800 bytes / 1 ms
        │ ~999 packets/sec
        ▼
Windows TcpLogger
        │
        │ TCP
        ▼
BufferPool
        │
        ▼
DiskWriter
        │
        ▼
LOG_00000001.bin
256.00 MB
```
## 實驗
# Client 端

```bash
D:\RaspberryPi\Raspberry_Pi_CM5\TcpDataClient> TcpDataClient.exe
[INFO] TcpDataClient starting.
[INFO] [CONFIG] Server=127.0.0.1:7777
[INFO] [CONFIG] Payload=800 bytes
[INFO] [CONFIG] Interval=1 ms
[INFO] [CONFIG] PacketCount=0
[INFO] TcpDataClient initialized.
[DEBUG] [DEBUG] MainLoop=42790044 State=4
[INFO] [STAT] Packets=1008 Delta=1008 Payload=806400 Total=830592
[INFO] [STAT] Rate=1008 pkt/s  830592 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=85086796 State=4
[INFO] [STAT] Packets=2008 Delta=1000 Payload=1606400 Total=1654592
[INFO] [STAT] Rate=1004 pkt/s  827296 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=127538102 State=4
[INFO] [STAT] Packets=3008 Delta=1000 Payload=2406400 Total=2478592
[INFO] [STAT] Rate=1002 pkt/s  826197 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=169430582 State=4
[INFO] [STAT] Packets=4008 Delta=1000 Payload=3206400 Total=3302592
[INFO] [STAT] Rate=1002 pkt/s  825648 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=211627846 State=4
[INFO] [STAT] Packets=5008 Delta=1000 Payload=4006400 Total=4126592
[INFO] [STAT] Rate=1001 pkt/s  825318 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=253752733 State=4
[INFO] [STAT] Packets=6008 Delta=1000 Payload=4806400 Total=4950592
[INFO] [STAT] Rate=1001 pkt/s  825098 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=295911051 State=4
[INFO] [STAT] Packets=7008 Delta=1000 Payload=5606400 Total=5774592
[INFO] [STAT] Rate=1001 pkt/s  824941 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=338133873 State=4
[INFO] [STAT] Packets=8008 Delta=1000 Payload=6406400 Total=6598592
[INFO] [STAT] Rate=1001 pkt/s  824824 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=380371566 State=4
[INFO] [STAT] Packets=9008 Delta=1000 Payload=7206400 Total=7422592
[INFO] [STAT] Rate=1000 pkt/s  824732 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=422632487 State=4
[INFO] [STAT] Packets=10008 Delta=1000 Payload=8006400 Total=8246592
[INFO] [STAT] Rate=1000 pkt/s  824659 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=464840975 State=4
[INFO] [STAT] Packets=11008 Delta=1000 Payload=8806400 Total=9070592
[INFO] [STAT] Rate=1000 pkt/s  824599 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=506034786 State=4
[INFO] [STAT] Packets=11996 Delta=988 Payload=9596800 Total=9884704
[INFO] [STAT] Rate=999 pkt/s  823725 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=547369306 State=4
[INFO] [STAT] Packets=12997 Delta=1001 Payload=10397600 Total=10709528
[INFO] [STAT] Rate=999 pkt/s  823809 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=589935664 State=4
[INFO] [STAT] Packets=14008 Delta=1011 Payload=11206400 Total=11542592
[INFO] [STAT] Rate=1000 pkt/s  824470 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=631040666 State=4
[INFO] [STAT] Packets=15007 Delta=999 Payload=12005600 Total=12365768
[INFO] [STAT] Rate=1000 pkt/s  824384 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=673134695 State=4
[INFO] [STAT] Packets=15997 Delta=990 Payload=12797600 Total=13181528
[INFO] [STAT] Rate=999 pkt/s  823845 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=714629955 State=4
[INFO] [STAT] Packets=17008 Delta=1011 Payload=13606400 Total=14014592
[INFO] [STAT] Rate=1000 pkt/s  824387 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=756935220 State=4
[INFO] [STAT] Packets=17998 Delta=990 Payload=14398400 Total=14830352
[INFO] [STAT] Rate=999 pkt/s  823908 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=799413895 State=4
[INFO] [STAT] Packets=19008 Delta=1010 Payload=15206400 Total=15662592
[INFO] [STAT] Rate=1000 pkt/s  824346 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=841654521 State=4
[INFO] [STAT] Packets=20007 Delta=999 Payload=16005600 Total=16485768
[INFO] [STAT] Rate=1000 pkt/s  824288 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=883441191 State=4
[INFO] [STAT] Packets=21008 Delta=1001 Payload=16806400 Total=17310592
[INFO] [STAT] Rate=1000 pkt/s  824313 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=925702995 State=4
[INFO] [STAT] Packets=22007 Delta=999 Payload=17605600 Total=18133768
[INFO] [STAT] Rate=1000 pkt/s  824262 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=966819453 State=4
[INFO] [STAT] Packets=22996 Delta=989 Payload=18396800 Total=18948704
[INFO] [STAT] Rate=999 pkt/s  823856 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=1008770525 State=4
[INFO] [STAT] Packets=23998 Delta=1002 Payload=19198400 Total=19774352
[INFO] [STAT] Rate=999 pkt/s  823931 bytes/s
[DEBUG] [STAT] TX activity detected.
[DEBUG] [DEBUG] MainLoop=1050395082 State=4
[INFO] [STAT] Packets=24996 Delta=998 Payload=19996800 Total=20596704
[INFO] [STAT] Rate=999 pkt/s  823868 bytes/s
[DEBUG] [STAT] TX activity detected.
...............
```

## 解說

這一段非常好，而且它提供了一個比前一次更完整的證據。

**這次是穩定 PASS，而且從 1,000 pkt/s 左右一路維持到 25,000 packets，沒有再出現 64 pkt/s。**

更重要的是：這次結果可以讓我們對昨天的「64 pkt/s」問題有更進一步的判斷。

---

# 一、先看最重要的結論

這次：

```text
Payload       = 800 bytes
Interval      = 1 ms
PacketCount   = 0 (Infinite)
```

實際：

```text
~999～1008 pkt/s
```

長時間：

```text
1000 pkt/s
```

而不是：

```text
64 pkt/s
```

所以目前 TcpDataClient 的傳輸能力：

> **已經完全符合我們第一階段要求。**

---

# 二、第一秒就已經超過 1000 pkt/s

第一筆：

```text
[STAT] Packets=1008 Delta=1008 Payload=806400 Total=830592
[STAT] Rate=1008 pkt/s  830592 bytes/s
```

驗算：

```text
1008 × 800
= 806,400 bytes
```

完全正確。

完整 packet：

```text
1008 × 824
= 830,592 bytes
```

也完全正確。

所以：

```text
800-byte payload
+
24-byte header
=
824-byte packet
```

完全沒有問題。

---

# 三、第二秒開始非常接近 1000 pkt/s

第二次：

```text
Packets=2008
Delta=1000
Rate=1004 pkt/s
```

第三次：

```text
Packets=3008
Delta=1000
Rate=1002 pkt/s
```

第四次：

```text
Packets=4008
Delta=1000
Rate=1002 pkt/s
```

第五次：

```text
Packets=5008
Delta=1000
Rate=1001 pkt/s
```

接下來：

```text
1001
1001
1001
1000
1000
1000
```

這就是非常漂亮的 1 ms transmission behavior。

---

# 四、為什麼第一秒是 1008，後面變成 1000？

這是正常現象，不代表 Client 超速或錯誤。

第一個 statistics interval 不一定恰好從：

```text
t = 0.000 sec
```

開始。

Client 啟動、socket connect、timer initialization、第一個 packet 建立等都會造成統計區間的邊界差異。

所以：

```text
1008 pkt/s
1004 pkt/s
1002 pkt/s
1001 pkt/s
```

慢慢收斂到：

```text
1000 pkt/s
```

是合理的。

真正值得看的是長期結果。

---

# 五、這次最漂亮的地方：長時間穩定

你一直跑到：

```text
Packets=24996
```

也就是將近：

```text
25,000 packets
```

這不是短時間偶然成功。

從：

```text
1008
```

一路：

```text
2008
3008
4008
5008
6008
7008
8008
9008
10008
11008
11996
12997
14008
15007
15997
17008
17998
19008
20007
21008
22007
22996
23998
24996
```

都維持在：

```text
~1000 packets/sec
```

這表示 scheduler 已經非常穩定。

---

# 六、MainLoop 再次證明不是瓶頸

第一秒：

```text
MainLoop=42,790,044
```

第二秒：

```text
MainLoop=85,086,796
```

差：

```text
~42.3 million
```

之後：

```text
127,538,102
169,430,582
211,627,846
...
```

大致都在：

```text
~42 million loops/sec
```

因此現在 Client 的運作模式非常清楚：

```text
MainLoop
~42 million/sec
       │
       │ 大量機會執行
       ▼
tcp_client_process()
       │
       ▼
1 ms scheduler
       │
       ▼
~1000 TX/sec
```

這完全合理。

---

# 七、Statistics 的數學也全部正確

例如：

```text
Packets=10008
Payload=8006400
Total=8246592
```

驗證：

```text
10008 × 800
= 8,006,400
```

正確。

再算：

```text
10008 × 824
= 8,246,592
```

也正確。

所以：

```text
Payload = 800 bytes
Header  = 24 bytes
Total   = 824 bytes
```

沒有任何 drift。

---

# 八、Rate=1000 pkt/s 時，真正的資料率是多少？

例如：

```text
Rate=1000 pkt/s
824 bytes/packet
```

所以完整 TCP protocol data：

```text
1000 × 824
=
824,000 bytes/sec
```

也就是：

```text
~824 KB/sec
```

而真正 application payload：

```text
1000 × 800
=
800,000 bytes/sec
```

所以目前完全符合我們最初定義的：

```text
800 bytes × 1000 packets/sec
=
800,000 bytes/sec
```

---

# 九、這次與上一份 64 pkt/s log 最大的差異

上一份：

```text
MainLoop
~100 million/sec

TX
~64 pkt/s
```

這一次：

```text
MainLoop
~42 million/sec

TX
~1000 pkt/s
```

注意一件非常重要的事情：

**這一次 MainLoop 反而比較低。**

上一份：

```text
~100 million loops/sec
```

現在：

```text
~42 million loops/sec
```

但是 TX：

```text
64 → 1000 pkt/s
```

這直接證明：

> **MainLoop 執行速度與 TX 速率沒有直接正相關。**

所以我們之前看到的：

```text
~100 million loops/sec
```

並不代表「Client 一定會傳得快」。

真正決定 TX 的，是：

```text
timer / scheduler / send path
```

這個證據非常有價值。

---

# 十、這也讓我對「64 pkt/s」有新的判斷

目前我們已經有：

### 測試 A

```text
MainLoop ≈ 100M/sec
TX ≈ 64 pkt/s
```

### 測試 B

```text
MainLoop ≈ 42M/sec
TX ≈ 1000 pkt/s
```

因此：

```text
MainLoop speed
≠
TX speed
```

這可以正式排除：

> 「MainLoop 不夠快，所以只能傳 64 pkt/s」

這個假設。

---

# 十一、QueryPerformanceCounter 修改顯然有效

目前成功的版本使用：

```c
QueryPerformanceCounter()
QueryPerformanceFrequency()
```

而不是：

```c
GetTickCount64()
```

這次測試再次證明這個修改是有效的。

目前的結果：

```text
~1000 packets/sec
```

而且持續：

```text
25,000 packets
```

這是非常強的實證。

---

# 十二、但我不會說「64 pkt/s 的原因已經百分之百查明」

這一點我要很嚴謹。

我們現在可以說：

> **使用 QPC 的版本可以穩定達到 1 ms / ~1000 pkt/s。**

但是我們還不能單憑這幾次測試完全證明：

> 「64 pkt/s 一定是 GetTickCount64 的唯一原因。」

因為我們剛才已經看到同一版本可能出現不同結果。

所以如果我們要真正把原因寫進工程紀錄，最好寫成：

> Windows high-resolution performance counter resolved the observed 1 ms transmission scheduling limitation in the tested configuration.

而不要寫：

> GetTickCount64 is definitely the only cause.

這樣工程紀錄會比較嚴謹。

---

# 十三、現在這份 Client log 可以當成 Phase 1 的基準測試

我建議你把這次數據保存起來。

因為這可以當作之後 Raspberry Pi CM5 測試的 baseline：

```text
Windows localhost baseline
```

### Client baseline

```text
Server       : 127.0.0.1:7777
Payload      : 800 bytes
Interval     : 1 ms
Packet       : 824 bytes
PacketCount  : Infinite

Measured:
~999–1000 pkt/s
~800,000 bytes/s payload
~824,000 bytes/s protocol data
```

之後：

```text
Windows Client
      ↓
Raspberry Pi CM5
```

我們就拿這個 baseline 比較。

如果 CM5 測試變成：

```text
1000 pkt/s
```

→ PASS。

如果變成：

```text
850 pkt/s
```

→ 開始調查 Server / Network。

如果變成：

```text
64 pkt/s
```

→ 就要立即調查 Server / TCP flow control / socket behavior。

---

# 十四、現在最重要的事情

你現在這個 Client 已經跑到：

```text
24996 packets
```

而且一直：

```text
999～1000 pkt/s
```

所以我會建議：

**不要再動 Client。**

這個版本現在就是我們的：

> **Windows TcpDataClient Reference Test Version**

接下來直接用它餵：

```text
TcpLogger
```

讓 Server 跑到：

```text
256 MB
```

再驗證：

```text
LOG_00000001.bin
```

然後再做：

```text
Ctrl+C / TCP disconnect
```

驗證最後不足 256 MB 的資料是否保存。

---

## 最後一句話

這次這份 log 比上一份更重要，因為它證明：

> **目前 TcpDataClient 的 800-byte / 1-ms transmission scheduler 可以長時間穩定運作在約 1000 pkt/s，而且 25,000 packets 的累積統計、payload、824-byte protocol packet 全部一致。**

因此現在：

**Client 不需要再修改。**

我們應該把它當成已經驗證成功的 Client baseline，接下來把注意力完全放到 **TcpLogger Server 的長時間接收、256 MB 切檔，以及最後不足 256 MB 的 flush 行為**。


---
# Server 端

```bash
D:\RaspberryPi\Raspberry_Pi_CM5\TcpLogger>build\TcpLogger.exe
[08:32:16.879] [SUCCESS] ========================================
[08:32:16.881] [SUCCESS] TCP Logger Server Version 1.0
[08:32:16.881] [SUCCESS] Listening Port : 7777
[08:32:16.881] [SUCCESS] Save Path : D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG
[08:32:16.881] [SUCCESS] ========================================
[08:32:17.510] [SUCCESS] BufferPool Initialized
[08:32:17.510] [BUFFER] Total=8  BufferSize=256.00 MB
[08:32:17.510] [BUFFER] Status  Total=8  Free=8
[08:32:17.510] [SUCCESS] RingBuffer Initialized
[08:32:17.511] [BUFFER] RingBuffer Queue=0/128  Head=0  Tail=0
[08:32:17.511] [SUCCESS] DiskWriter Started
[08:32:17.512] [INFO] DiskWriter Thread Running
[08:32:17.513] [SOCKET] Listen Port=7777
[08:32:17.513] [INFO] TcpServer Started
[08:32:17.513] [SUCCESS] Waiting for STM32 Client...
[08:32:22.685] [SOCKET] Client Connected : 127.0.0.1
[08:32:22.685] [BUFFER] Acquire Buffer #1  Free=7
[08:37:49.070] [BUFFER] Acquire Buffer #2  Free=6
[08:37:49.335] [DISK] D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG/20260806/LOG_00000001.bin  256.00 MB
[08:37:49.336] [BUFFER] Release Buffer #1  Free=7
[08:37:49.336] [STAT] Write Speed=0.77 MB/s  Total=256.00 MB  Files=1  Queue=0
[08:43:16.037] [BUFFER] Acquire Buffer #3  Free=6
[08:43:16.288] [DISK] D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG/20260806/LOG_00000002.bin  256.00 MB
[08:43:16.289] [BUFFER] Release Buffer #2  Free=7
[08:43:16.289] [STAT] Write Speed=0.78 MB/s  Total=256.00 MB  Files=2  Queue=0
[08:48:42.852] [BUFFER] Acquire Buffer #4  Free=6
[08:48:43.099] [DISK] D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG/20260806/LOG_00000003.bin  256.00 MB
[08:48:43.099] [BUFFER] Release Buffer #3  Free=7
[08:48:43.100] [STAT] Write Speed=0.78 MB/s  Total=256.00 MB  Files=3  Queue=0
[08:54:09.962] [BUFFER] Acquire Buffer #5  Free=6
[08:54:10.212] [DISK] D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG/20260806/LOG_00000004.bin  256.00 MB
[08:54:10.212] [BUFFER] Release Buffer #4  Free=7
[08:54:10.212] [STAT] Write Speed=0.78 MB/s  Total=256.00 MB  Files=4  Queue=0
[08:59:36.494] [BUFFER] Acquire Buffer #6  Free=6
[08:59:36.750] [DISK] D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG/20260806/LOG_00000005.bin  256.00 MB
[08:59:36.750] [BUFFER] Release Buffer #5  Free=7
[08:59:36.751] [STAT] Write Speed=0.78 MB/s  Total=256.00 MB  Files=5  Queue=0
[08:59:46.001] [SOCKET] Client Disconnected : 127.0.0.1
[08:59:46.006] [DISK] D:/RaspberryPi/Raspberry_Pi_CM5/TcpLogger/LOG/20260806/LOG_00000006.bin  7.39 MB
[08:59:46.006] [BUFFER] Release Buffer #6  Free=8
[08:59:56.079] [INFO] Stopping Server...
[08:59:56.080] [INFO] TcpServer Shutdown Requested
[08:59:56.186] [INFO] TcpServer Stopped
[08:59:56.187] [INFO] DiskWriter Shutdown Requested
[08:59:56.187] [INFO] RingBuffer Stopped
[08:59:56.187] [INFO] DiskWriter Thread Exit
[08:59:56.188] [INFO] DiskWriter Stopped
[08:59:56.188] [SUCCESS] Server Stopped
```
## 解說

**從你現在提供的 Client + Server 兩邊實際測試結果來看，我認為第一階段的核心目標已經達成。**

而且這次不是「看起來可以」，而是已經有完整的實測證據：

```text
Windows TcpDataClient
        │
        │ 800 bytes / 1 ms
        │ ~999 packets/sec
        ▼
Windows TcpLogger
        │
        │ TCP
        ▼
BufferPool
        │
        ▼
DiskWriter
        │
        ▼
LOG_00000001.bin
256.00 MB
```

---

# 一、Server log 已經證明什麼？

最關鍵的是這幾行：

```text
[08:32:22.685] [SOCKET] Client Connected : 127.0.0.1
[08:32:22.685] [BUFFER] Acquire Buffer #1  Free=7
```

代表 Client 成功連線，而且 Server 開始取得第一個 256 MB Buffer。

接下來大約 5 分 26 秒後：

```text
[08:37:49.070] [BUFFER] Acquire Buffer #2  Free=6
[08:37:49.335] [DISK] .../LOG_00000001.bin  256.00 MB
[08:37:49.336] [BUFFER] Release Buffer #1  Free=7
[08:37:49.336] [STAT] Write Speed=0.77 MB/s  Total=256.00 MB  Files=1  Queue=0
```

這幾行非常重要。

它證明：

### ① 第一個 256 MB Buffer 已經填滿

```text
Buffer #1
```

從：

```text
08:32:22.685
```

到：

```text
08:37:49.070
```

約：

```text
5 分 26 秒
```

這和我們從 Client 的：

```text
~800,000 bytes/sec payload
```

預估的時間幾乎完全吻合。

---

### ② Server 成功建立 256 MB binary file

```text
LOG_00000001.bin  256.00 MB
```

這是第一階段最重要的驗收結果。

不是只有收到資料，而是：

```text
TCP
 ↓
BufferPool
 ↓
DiskWriter
 ↓
256 MB binary file
```

整條鏈路已經跑通。

---

### ③ Buffer rollover 已經開始工作

這一行：

```text
[BUFFER] Acquire Buffer #2  Free=6
```

非常重要。

代表：

```text
Buffer #1
    ↓
填滿 256 MB
    ↓
交給 DiskWriter
    ↓
開始使用 Buffer #2
```

也就是我們原本設計的：

```text
Buffer #1 → File #1
Buffer #2 → File #2
Buffer #3 → File #3
...
```

已經開始按照設計運作。

---

### ④ DiskWriter 成功寫出檔案

```text
[DISK] ... LOG_00000001.bin 256.00 MB
```

以及：

```text
[STAT] Write Speed=0.77 MB/s
[STAT] Total=256.00 MB
[STAT] Files=1
[STAT] Queue=0
```

這也很漂亮。

尤其：

```text
Queue=0
```

表示目前測試中 DiskWriter 沒有產生持續性的 backlog。

---

# 二、因此第一階段可以宣布 PASS

我們原本定義的第一階段：

```text
電腦內部自測

Windows TcpDataClient
        ↓
TCP
        ↓
Windows TcpLogger Server
        ↓
BufferPool
        ↓
DiskWriter
        ↓
每 256 MB 建立一個檔案
```

現在實際結果：

| 測試項目                           | 結果        |
| ------------------------------ | --------- |
| Client 800-byte payload        | ✅ PASS    |
| Client 1 ms interval           | ✅ PASS    |
| 約 1000 packet/s                | ✅ PASS    |
| Protocol packet 824 bytes      | ✅ PASS    |
| TCP localhost 傳輸               | ✅ PASS    |
| Server 接收                      | ✅ PASS    |
| BufferPool                     | ✅ PASS    |
| RingBuffer                     | ✅ PASS    |
| DiskWriter                     | ✅ PASS    |
| 256 MB file creation           | ✅ PASS    |
| Buffer #1 → Buffer #2 rollover | ✅ PASS    |
| `LOG_00000001.bin`             | ✅ 256 MB  |
| Queue 穩定性                      | ✅ Queue=0 |

所以我會正式把目前狀態定義為：

> **第一階段：Windows Client → Windows TcpLogger Server 本機整合測試 PASS。**

---

# 三、不過有一個小地方我要精確說明

我們原始的完整需求還包括：

> 如果 TCP 被中斷，未滿 256 MB 的剩餘資料也必須保存成最後一個檔案。

**這一項目前這份 log 還沒有測到。**

目前我們已經驗證：

```text
256 MB
→
完整檔案
```

也驗證了：

```text
Buffer #1
→
Buffer #2
```

但還沒有在這次測試中驗證：

```text
例如：

256 MB
+
173 MB
+
Ctrl+C / TCP disconnect
        ↓
LOG_00000001.bin 256 MB
LOG_00000002.bin 173 MB
```

所以我會把第一階段分成：

### 核心資料傳輸與 256 MB 切檔

**PASS ✅**

### 中斷時最後不足 256 MB 的檔案保存

**尚待專門驗證 ⏳**

這不影響我們說「第一階段核心整合成功」，但 README 裡最好誠實記錄。

---

# 四、這次 Client 修改的真正原因

昨天 Client 曾經出現：

```text
Rate=64 pkt/s
```

目標卻是：

```text
1000 pkt/s
```

當時 MainLoop 已經證明：

```text
~100,000,000 loops/sec
```

所以我們判斷：

> 問題不是 MainLoop 執行速度不足。

接著檢查 Client 的時間控制。

原本 Windows 平台使用：

```c
GetTickCount64()
```

來取得毫秒時間。

這對一般 Windows 應用程式的時間測量沒有問題，但我們現在的需求是：

```text
1 ms periodic transmission
```

需要更高解析度的時間基準。

因此我們沒有修改整個 TcpClient 架構，而是**只修改 Platform Time Layer**。

---

# 五、這次的修改方式

原本概念：

```text
Windows
    ↓
GetTickCount64()
    ↓
毫秒時間
    ↓
1 ms scheduler
```

修改成：

```text
Windows
    ↓
QueryPerformanceCounter()
    ↓
QueryPerformanceFrequency()
    ↓
高解析度時間
    ↓
1 ms scheduler
```

使用：

```c
static LARGE_INTEGER s_qpc_frequency;
```

初始化時：

```c
QueryPerformanceFrequency(
    &s_qpc_frequency);
```

取得目前時間：

```c
QueryPerformanceCounter(
    &counter);
```

再將 counter 轉換成毫秒：

```c
((uint64_t)counter.QuadPart * 1000ULL) /
(uint64_t)s_qpc_frequency.QuadPart;
```

---

# 六、最重要的是：我們沒有破壞原本架構

這一點我非常建議 README 記錄。

這次不是：

```text
❌ 重寫 TcpClient
❌ 修改 protocol
❌ 修改 packet size
❌ 修改 send architecture
❌ 修改 Server
❌ 加入 Windows-specific code 到核心 TCP logic
```

而是：

```text
Platform-specific time implementation
```

也就是：

```text
tcp_client.c
        │
        ├── Windows
        │      └── QueryPerformanceCounter()
        │
        ├── Linux
        │      └── Linux time implementation
        │
        └── STM32
               └── STM32 platform timer
```

這符合我們原本的：

```text
Cross Platform
Single Responsibility
Platform Abstraction
```

設計原則。

---

# 七、這次修改後的實際改善

修改前：

```text
Payload = 800 bytes
Interval = 1 ms

實際：
~64 packets/sec
~52.7 KB/sec
```

修改後：

```text
Payload = 800 bytes
Interval = 1 ms

實際：
~999 packets/sec
~800 KB/sec payload
~824 KB/sec total TCP data
```

改善非常巨大。

大約：

```text
999 / 64
≈ 15.6 倍
```

這也正好解釋了為什麼昨天看到：

```text
~64 packets/sec
```

會讓我們懷疑約：

```text
15.6 ms
```

的時間週期。

---

# 八、README 直接加入這一段

這是可以直接放進 GitHub README 的正式版本：

## Windows TCP Client Timing Improvement

During the initial Windows integration test, the TcpDataClient was configured to transmit an 800-byte payload every 1 ms. However, the actual transmission rate was only approximately 64 packets/sec.

The main loop was already running at approximately 100 million iterations/sec, so the main loop execution speed was not the limiting factor.

### Root Cause

The Windows platform timing implementation used `GetTickCount64()` as the time source for the 1 ms transmission scheduler.

For a periodic transmission requirement of 1 ms, a higher-resolution performance counter is more appropriate.

### Modification

The Windows timing implementation in `tcp_client.c` was changed from `GetTickCount64()` to the Windows high-resolution performance counter API:

* `QueryPerformanceFrequency()`
* `QueryPerformanceCounter()`

The counter frequency is initialized once during `tcp_client_initialize()`, and the current counter value is converted to milliseconds by integer arithmetic.

The transmission scheduler itself was not redesigned.

The existing scheduling mechanism using `next_send_time_ms` was retained:

```c
do
{
    s_client.next_send_time_ms +=
        (uint64_t)s_client.config.send_interval_ms;

} while (s_client.next_send_time_ms <= now);
```

This preserves the existing periodic scheduling architecture while providing a higher-resolution Windows time base.

### Test Result

Before the modification:

```text
Payload        : 800 bytes
Interval       : 1 ms
Actual Rate    : ~64 packets/sec
Payload Rate   : ~52.7 KB/sec
```

After the modification:

```text
Payload        : 800 bytes
Interval       : 1 ms
Actual Rate    : ~999 packets/sec
Payload Rate   : ~800 KB/sec
Total TCP Rate : ~824 KB/sec
```

A continuous local Windows test successfully transmitted approximately 40,000 packets while maintaining approximately 999 packets/sec.

### Integration Test Result

The Windows TcpDataClient was then tested against the Windows TcpLogger server using:

```text
TcpDataClient
    |
    | TCP / 127.0.0.1:7777
    v
TcpLogger
    |
    v
BufferPool
    |
    v
DiskWriter
    |
    v
256 MB binary file
```

The server successfully received the continuous data stream and generated:

```text
LOG_00000001.bin    256.00 MB
```

The BufferPool also successfully transitioned from Buffer #1 to Buffer #2 after the first 256 MB buffer was completed.

### Current Status

**Phase 1 — Windows Client/Server Local Integration Test: PASS**

Verified:

* 800-byte application payload
* 1 ms transmission interval
* Approximately 1000 packets/sec
* 824-byte complete protocol packet
* Continuous TCP transmission
* Windows TcpLogger server reception
* BufferPool operation
* RingBuffer operation
* DiskWriter operation
* 256 MB binary file generation
* Buffer rollover to the next buffer

The remaining dedicated test is graceful handling of a TCP interruption while the current buffer contains less than 256 MB. The incomplete final data must be preserved as the final binary file.

---

# 九、我特別建議保留這次測試數據

不要只在 README 寫：

```text
Phase 1 PASS
```

這次其實很值得留下實際數據。

尤其是：

```text
Client:
~999 pkt/s
```

以及：

```text
Server:
LOG_00000001.bin 256.00 MB
```

這兩個數據放在一起非常有說服力。

因為它證明不是只有：

> Client 說自己送了。

而是：

> **Client 實際以約 1 ms 週期傳送，Server 實際接收並最終寫出了 256 MB binary file。**

---

## 現在的專案進度

我會把目前進度定義為：

```text
Phase 1
Windows Client
     ↓
Windows TcpLogger
     ↓
256 MB File
     ↓
             ✅ PASS
```

下一個我們應該驗證的是同一個 Phase 1 的最後一個邊界案例：

```text
Client 持續傳送
     ↓
尚未滿 256 MB
     ↓
Ctrl+C / TCP disconnect
     ↓
Server flush remaining data
     ↓
產生最後不足 256 MB 的 bin
```

**這項測試完成後，我就會認為 Phase 1 的功能驗收真正完整。**

然後才進入我們原本規劃的：

```text
Phase 2
Windows TcpDataClient
        ↓
Ethernet
        ↓
Raspberry Pi CM5 TcpLogger
```

這樣的順序是最穩妥的，現在**完全不需要因為速度問題去修改 Client 或 Server 核心程式**。

---

> 是的，這份**完整測試結果已經比前面的測試更完整**。現在我們可以對 Windows 本機第一階段做出非常明確的工程結論。

## 先給你結論

### ✅ Phase 1：Windows Client → Windows TcpLogger Server 整合測試 PASS

你這次實際驗證了：

```text
TcpDataClient
    │
    │ 800-byte payload / 1 ms
    │ ≈ 1000 packets/sec
    ▼
TCP localhost
    ▼
TcpLogger Server
    │
    ├── BufferPool
    ├── RingBuffer
    └── DiskWriter
          │
          ├── 256 MB file #1
          ├── 256 MB file #2
          ├── 256 MB file #3
          ├── 256 MB file #4
          ├── 256 MB file #5
          └── remaining 7.39 MB file #6
```

而且這次不是只產生一個 256 MB 檔案。

你實際連續產生：

```text
LOG_00000001.bin   256 MB
LOG_00000002.bin   256 MB
LOG_00000003.bin   256 MB
LOG_00000004.bin   256 MB
LOG_00000005.bin   256 MB
LOG_00000006.bin     7.39 MB
```

也就是：

```text
5 × 256 MB
+
7.39 MB
=
1287.39 MB
```

大約 **1.26 GiB** 的資料。

這已經是相當有意義的整合測試，不是單純跑幾秒鐘看看。

---

# 1. 最重要的：256 MB 切檔真的成功

第一個：

```text
[08:37:49.335] [DISK] ... LOG_00000001.bin  256.00 MB
```

第二個：

```text
[08:43:16.288] [DISK] ... LOG_00000002.bin  256.00 MB
```

第三個：

```text
[08:48:43.099] [DISK] ... LOG_00000003.bin  256.00 MB
```

第四個：

```text
[08:54:10.212] [DISK] ... LOG_00000004.bin  256.00 MB
```

第五個：

```text
[08:59:36.750] [DISK] ... LOG_00000005.bin  256.00 MB
```

這表示：

> **TcpLogger 可以連續接收資料，並且每累積滿 256 MB 就完成一次檔案切換。**

這部分已經不是推測，而是實際跑出來了。

---

# 2. 更重要的是 BufferPool 的切換正常

例如第一個 256 MB：

```text
[BUFFER] Acquire Buffer #2  Free=6
[DISK] ... LOG_00000001.bin  256.00 MB
[BUFFER] Release Buffer #1  Free=7
```

這代表：

```text
Buffer #1
    ↓
已經裝滿 256 MB
    ↓
交給 DiskWriter
```

同時：

```text
Buffer #2
    ↓
繼續接收新的 TCP data
```

然後：

```text
Buffer #1
    ↓
DiskWriter 寫完
    ↓
Release
    ↓
重新回到 Free Pool
```

這正是我們之前設計：

```text
BufferPool
+
RingBuffer
+
DiskWriter Thread
```

的目的。

---

# 3. 而且連續切了 5 個 256 MB，Free buffer 一直正常

你可以看到：

```text
#1 → #2
#2 → #3
#3 → #4
#4 → #5
#5 → #6
```

而每次：

```text
Free=6
```

然後寫完：

```text
Free=7
```

最後 Client disconnect：

```text
[BUFFER] Release Buffer #6  Free=8
```

這個：

```text
Free=8
```

非常漂亮。

因為一開始：

```text
Total=8
Free=8
```

最後：

```text
Free=8
```

也就是：

> **所有 Buffer 最終都成功回收到 BufferPool，沒有看到 buffer leak 或卡死。**

---

# 4. 最後不足 256 MB 的資料也成功保存

這是這次測試另一個非常重要的 PASS。

Client 最後按：

```text
Ctrl+C
```

Server：

```text
[08:59:46.001] [SOCKET] Client Disconnected : 127.0.0.1
[08:59:46.006] [DISK] ... LOG_00000006.bin  7.39 MB
[08:59:46.006] [BUFFER] Release Buffer #6  Free=8
```

這正好符合我們最初要求：

```text
如果 connection 中斷：

剩餘資料 < 256 MB
        ↓
不能丟掉
        ↓
仍然寫成最後一個檔案
```

實際結果：

```text
LOG_00000006.bin
7.39 MB
```

**PASS。**

---

# 5. Client Disconnect → Server Flush → Buffer Release

這三件事情的時間非常接近：

```text
08:59:46.001
Client Disconnected

08:59:46.006
LOG_00000006.bin 7.39 MB

08:59:46.006
Buffer #6 Release
```

只有大約：

```text
5 ms
```

這代表 TCP connection 結束後，Server 能夠處理最後殘留 buffer。

這是非常重要的功能。

---

# 6. Server 最後 Shutdown 也正常

接下來：

```text
[08:59:56.079] [INFO] Stopping Server...
[08:59:56.080] [INFO] TcpServer Shutdown Requested
[08:59:56.186] [INFO] TcpServer Stopped
[08:59:56.187] [INFO] DiskWriter Shutdown Requested
[08:59:56.187] [INFO] RingBuffer Stopped
[08:59:56.187] [INFO] DiskWriter Thread Exit
[08:59:56.188] [INFO] DiskWriter Stopped
[08:59:56.188] [SUCCESS] Server Stopped
```

這表示：

```text
TcpServer
    ↓
停止接收
    ↓
RingBuffer
    ↓
停止
    ↓
DiskWriter
    ↓
Thread Exit
    ↓
Server Stopped
```

完整 shutdown sequence 也正常。

---

# 7. 更值得注意：DiskWriter 並沒有阻塞 TCP 接收

你的每個 256 MB 檔案寫入：

```text
0.77 MB/s
0.78 MB/s
```

雖然這個「Write Speed」數字本身看起來很低，但**不要把它理解成 TCP 接收速度只有 0.78 MB/s**。

這裡的 statistics 是 DiskWriter 的寫入統計方式，不是 TCP receive throughput。

而真正重要的是：

```text
Acquire Buffer #N
        ↓
繼續接收
        ↓
下一個 Buffer
        ↓
DiskWriter 寫上一個 Buffer
```

你的測試可以持續產生：

```text
256 MB
256 MB
256 MB
256 MB
256 MB
7.39 MB
```

所以目前的架構確實達到了我們想要的：

```text
Receive
   │
   ▼
BufferPool
   │
   ▼
RingBuffer
   │
   ├──────────► 下一個 Buffer 繼續接收
   │
   ▼
DiskWriter
```

---

# 8. 但是「完全不會遺漏」還差最後一個驗證

這次我可以比之前更有信心地說：

> **目前測試過程沒有觀察到資料遺漏。**

而且測試量已經超過：

```text
1 GB
```

但如果我們要在 GitHub README 寫：

> **Zero data loss guaranteed**

我還是建議不要現在就這樣寫。

原因很簡單：

目前 Server log 證明：

```text
Buffer → Disk
```

完整。

但我們還沒有拿：

```text
Client transmitted packet count
```

跟：

```text
Server saved byte count
```

做**獨立的數學驗證**。

---

# 9. 不過這次已經足以正式宣布 Phase 1 功能完成

我會把目前的狀態定義成：

```text
PHASE 1
Windows localhost integration test

                    STATUS
────────────────────────────────
800-byte payload       PASS
1 ms interval          PASS
~1000 packets/sec      PASS
TCP transmission       PASS
TCP server receive     PASS
BufferPool             PASS
RingBuffer             PASS
DiskWriter             PASS
256 MB file rotation   PASS
Multiple files         PASS
Partial final file     PASS
Client disconnect      PASS
Buffer release         PASS
Server shutdown        PASS
────────────────────────────────
PHASE 1                 PASS
```

### 實際產生：

```text
LOG_00000001.bin   256.00 MB
LOG_00000002.bin   256.00 MB
LOG_00000003.bin   256.00 MB
LOG_00000004.bin   256.00 MB
LOG_00000005.bin   256.00 MB
LOG_00000006.bin     7.39 MB
```

這已經足以讓我們進入下一階段。

---

# 10. 我建議你現在不要再修改 TcpDataClient

目前 Client 已經證明：

```text
800 bytes
1 ms
~1000 packets/sec
```

長時間穩定。

Server 也證明：

```text
接收
→ BufferPool
→ RingBuffer
→ DiskWriter
→ 256 MB rotation
→ partial file
→ disconnect flush
```

全部跑通。

所以現在：

> **不要再為了「看起來更漂亮」去修改 Client 或 Server。**

這個版本應該先建立 Git checkpoint / tag。

例如：

```text
Phase-1-Windows-Integration-PASS
```

---

# 11. 下一階段就是我們原本規劃的

現在可以正式往：

```text
Windows TcpDataClient
        │
        │ TCP
        ▼
Raspberry Pi CM5
        │
        ▼
TcpLogger
```

前進。

也就是：

### Phase 2

```text
Windows TcpDataClient
        │
        │ 800 bytes / 1 ms
        ▼
Raspberry Pi CM5
        │
        ▼
Linux TcpLogger
        │
        ▼
256 MB files
```

這一步才會真正驗證：

```text
Windows → Ethernet → CM5 → Linux TCP Server
```

---

## 最後，我給你一個可以正式記錄在開發紀錄裡的結論

> **Phase 1 Windows localhost integration test is PASS.**
>
> The Windows TcpDataClient successfully generated and transmitted an 800-byte application payload at a 1 ms interval, maintaining approximately 1,000 packets/sec over an extended test.
>
> TcpLogger successfully received the continuous TCP stream through the BufferPool/RingBuffer pipeline while the DiskWriter asynchronously generated consecutive 256 MB binary files.
>
> The test successfully generated five consecutive 256 MB files, followed by a 7.39 MB final partial file after the client was stopped with Ctrl+C. All buffers were successfully released, and the server completed its shutdown sequence normally.
>
> No data loss or receive interruption was observed during this integration test.

**所以，Herman，這一次我會正式把「Windows Client → Windows Server → 256 MB continuous file logging」判定為第一階段完成。**

下一步不是再修這一階段，而是**保留目前版本，進入 Raspberry Pi CM5 Linux Server 整合測試。**

















