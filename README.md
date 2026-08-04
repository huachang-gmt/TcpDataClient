


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
