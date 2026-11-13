# STM32 Bluetooth SPP usage guide

## 1. What is available

The 0.1.0 library layout is hardware tested on STM32F746G-DISCO with the reported
CSR-compatible 0A12:0001 adapter. This branch develops 0.2.0: CubeMX configuration
for the bond sector, explicit flash maintenance, and the timer/callback example.
The published 0.1.0 pack does not contain these new configuration controls.
This is a community package; ST catalog inclusion and upstream acceptance are
separate processes. Other controller families and other MCU families need testing.

The same C library can be used from a CubeMX-generated project or a manually
managed Keil project. Keep it as a source dependency; do not copy its protocol
implementation into application files.

Baseline USB Host configuration from the working example (usbh_conf.h / CubeMX
USB_HOST parameters):

| Setting | Example value |
| --- | --- |
| USBH_MAX_NUM_ENDPOINTS | 8 |
| USBH_MAX_NUM_INTERFACES | 4 |
| USBH_MAX_SIZE_CONFIGURATION | 512 |
| USBH_MAX_NUM_SUPPORTED_CLASS | 1 |
| USBH_USE_OS | 0 |

The tested HS/ULPI integration disables HCD DMA. Do not enable DMA/cache features
in a new project and assume they are qualified: DMA-accessible RAM, buffer
alignment and Cortex-M7 cache coherency need board/middleware integration work.
Keep the host IRQ/HAL tick service working and poll continuously. The library
supplies the Bluetooth class, not clocks, ULPI wiring, VBUS hardware or USB HAL.


## 2. Execution model: like an asynchronous UART byte transport

```text
main loop / one owning task
    USBH_Process(host)       -> receive callback with transient bytes
    AppBT_Process()          -> BT_Library_Process(), serial event, one TX attempt
    application parser      <- AppBT_Read() copies queued received bytes

hardware timer ISR
    AppBT_RequestTxFromISR() -> increments a request counter only

explicit maintenance window, disconnected
    BT_Library_SaveBonds()   -> one synchronous flash-record write at most
```

`BT_Library_Send(data, length)` copies the entire span into its bounded TX queue
and returns immediately with 1 (accepted) or 0 (closed/invalid/full). It does not
wait for USB, RFCOMM credits or an acknowledgement. Accepted is not delivered.
Retry a rejected span later; do not busy-wait. Pending serial storage is 1024
bytes and negotiated RFCOMM payload is at most 120 bytes per frame. Split large
transfers and handle backpressure. The link can disconnect after acceptance.

All library/USB APIs and callbacks belong to one cooperative main-loop/task
context. Callbacks do not run inside a UART-style interrupt. Do not call Send,
Process, registration or flash functions from the timer ISR, and do not access
them simultaneously from multiple tasks. IRQ code posts work; the owner performs
it. The provided example has exactly one producer for its aligned 32-bit timer
counter; requests may coalesce while the connection is busy or disconnected.

Receive callback data is valid only until that callback returns. Consume it or
copy it. RFCOMM is a stream: one application message may arrive in pieces, and
several messages may arrive together. Reuse your UART/USB framing parser with
length/delimiter/CRC handling; do not treat callback boundaries as message frames.
Keep parser state and reply routing separate for each active transport.

Normal Process/Send/receive dispatch contains no flash programming or polling
wait for transport completion. Startup includes controller initialization and
bond-journal recovery, which is separate from steady-state serial operation.
The supplied HAL flash program/erase routines are synchronous. SaveBonds and
factory erase are explicit exceptions, not hard-real-time operations. For hard
latency bounds during persistence, provide a suitable asynchronous external
storage design; changing a function name to an interrupt HAL call is not proof
of system-wide timing safety.

## 3. Choose and reserve paired-device storage FIRST

The F746 port accepts complete sectors at the following addresses:

| Sector | Start | Bytes | Availability |
| --- | --- | --- | --- |
| 0 | 0x08000000 | 0x8000 (32 KiB) | 512 KiB / 1 MiB parts |
| 1 | 0x08008000 | 0x8000 | 512 KiB / 1 MiB parts |
| 2 | 0x08010000 | 0x8000 | 512 KiB / 1 MiB parts |
| 3 | 0x08018000 | 0x8000 | 512 KiB / 1 MiB parts |
| 4 | 0x08020000 | 0x20000 (128 KiB) | 512 KiB / 1 MiB parts |
| 5 | 0x08040000 | 0x40000 (256 KiB) | 512 KiB / 1 MiB parts |
| 6 | 0x08080000 | 0x40000 | 1 MiB parts only |
| 7 | 0x080C0000 | 0x40000 | 1 MiB parts only |

Default: sector 7 on the DISCO board's 1 MiB device. A typical 512 KiB project
can reserve sector 5 and limit its firmware to the space below 0x08040000.
Earlier sectors are only suitable for an intentionally partitioned linker layout;
sector 0 normally contains the boot vectors and must not be selected casually.
The port rejects non-boundary addresses, size/sector mismatches, unsupported
flash-size values, and regions outside the physical flash-size fuse. It does
NOT know your application's linker partition and cannot prove it is reserved.
It assumes the supplied board's flash programming voltage/word-write conditions.
Review these before using it on another board or supply voltage.

For a simple image starting at 0x08000000, restrict the flash load region to:

```text
maximum image bytes = bond-sector start - 0x08000000
sector 7 example:   0x000C0000
sector 5 example:   0x00040000
```

In Keil, set the IROM1 region size under Options for Target > Target if the linker
uses the target memory map. If using a custom scatter file, restrict BOTH the
load region and flash execution region there. A generated scatter file can be
regenerated from target settings; changing only that generated file is fragile.
Verify the final map file: the load region must end before the bond sector. For
GCC, adjust the FLASH region in the linker script accordingly. A configuration
header or CubeMX dropdown does not edit these linker settings for you.

Configure the programmer for necessary application-sector erases, not full-chip
erase, when preserving bonds. No ordinary library operation erases the journal.
When it fills, saving remains pending and BT_Bonds_Full is set; deliberate
maintenance is needed. Changing the address does not migrate or erase old bonds:
plan for a new empty journal/re-pairing and protect the old region during updates.
The existing 36-byte on-flash record format is unchanged by this branch.

## 4. CubeMX project walkthrough

1. Build this branch with `./Tools/build-pack.ps1`, then install the resulting
   MatinKarimi.STM32Bluetooth.0.2.0.pack via CubeMX's software package manager,
   From Local. The existing public release is 0.1.0; use the newly built preview.
2. Create a new project for your exact STM32F746 device/board. Configure a working
   USB Host controller, clocks, PHY, pins, VBUS switching and IRQs. The DISCO example
   uses USB OTG HS with external ULPI PHY. Host FS is not separately qualified by
   this project. The Bluetooth class does not initialize board wiring for you.
3. Open Additional Software/Software Packs, select the MatinKarimi STM32Bluetooth
   ClassicSPP component and the F746Flash port, then confirm the selection.
   Under Middleware and Software Packs > STM32Bluetooth, check both USB modes
   to activate them. In its Parameter Settings page choose **Reserved bond flash
   address**, device name, server channel,
   maximum paired devices and the explicit development pairing policy.
4. For the initial phone echo test, enable development Just Works pairing in a
   controlled setup. The generic setting is disabled. A product with user-approved
   pairing needs its own policy; automatic pairing is not enabled silently.
5. Generate a Keil project. Confirm STM32Bluetooth/App/bt_user_config.h contains
   the chosen
   address and name. Storage size and erase sector are derived together by
   bt_f746_flash_layout.h; do not edit the generated header instead of CubeMX.
   Ensure the generated STM32Bluetooth/App header directory and library Inc/ are in
   the compiler include paths. Version 0.2.0 requires bt_user_config.h directly;
   the old BT_USE_USER_CONFIG compiler define is no longer necessary.
6. Reserve the same sector in the actual linker configuration as described above.
   Add Examples/PeriodicCallback/app_bt_periodic.c and its header to the application
   (or write your own callbacks). Do not compile the separate echo example too.
7. Make exactly ONE Bluetooth class registration between USBH_Init and USBH_Start:

```c
/* usb_host.c: include app_bt_periodic.h in the USER CODE includes. */
if (USBH_Init(&hUsbHostHS, USBH_UserProcess, HOST_HS) != USBH_OK) {
    Error_Handler();
}
if (AppBT_Register(&hUsbHostHS) != USBH_OK) { Error_Handler(); }
if (USBH_Start(&hUsbHostHS) != USBH_OK) { Error_Handler(); }
```

Replace a conflicting generated HID/MSC registration in this single-class
example. Do not register Bluetooth twice and do not register before USBH_Init.
Do not replace your USBH_UserProcess callback. This preview generates the library
configuration but does not automatically wire this application-specific ordering.
Regenerating USB Host code may restore its selected built-in class: review this
point after every regeneration until full integration templates are validated.

8. Poll from your existing main-loop USER CODE section:

```c
for (;;) {
    MX_USB_HOST_Process();  /* already calls USBH_Process; do not call it twice */
    AppBT_Process();        /* already calls BT_Library_Process */
    /* Read/parser work and other nonblocking application services here. */
}
```

For an application without MX_USB_HOST_Process, call USBH_Process(host) yourself.
Do not substitute a slow timer for continuous USB host polling. The timer controls
application transmissions only.

## 5. Keil-only project walkthrough

1. Start from working board clock/USB Host/HAL initialization. Add library Inc/
   to include paths, every Src/*.c to one group, and exactly one port file:
   Ports/STM32F746/bt_port_stm32f746.c. ST USB Host Core, USB low-level glue,
   STM32 HAL/HCD and CMSIS are supplied by your project.
2. Copy Templates/bt_user_config.h into the application's include directory.
   Set BT_DEVICE_NAME, BT_SPP_RFCOMM_CHANNEL, BT_PAIRING_TEST_MODE,
   BT_BONDS_MAX_DEVICES and BT_BONDS_FLASH_START. Keep the layout-header include
   and BT_BONDS_STORAGE_BYTES derivation; do not maintain an unrelated erase number.
3. Reserve that sector in Keil/linker/programmer settings and inspect the map.
4. Add the PeriodicCallback directory to the include paths when using that
   example. Add ONE of the example applications or your own registered callbacks. Use
   the same USBH_Init -> AppBT_Register -> USBH_Start ordering and continuous
   main-loop polling shown in the CubeMX walkthrough.
5. For a different storage device or MCU family, supply bt_port.h functions
   instead of compiling the F746 flash port. Port read/write bounds, torn-record
   protection and commit-last behavior are mandatory contracts. For a non-F746
   storage port, replace the F746 layout include in your project configuration
   with the correct BT_BONDS_STORAGE_BYTES for that storage. USB HAL behavior
   and a new MCU/controller combination still require qualification.

### Reuse across several products

Keep one pinned library version as a Git submodule, release archive or shared
source folder. Each product owns its bt_user_config.h (or CubeMX parameters),
board USB/VBUS initialization, linker/programmer reservation and application
callbacks. Do not edit shared protocol sources for different names or bond sectors.
The pack copies sources into generated projects; after changing the installed pack,
regenerate/rebuild and review the version instead of assuming those copies updated.

These examples use USBH_USE_OS == 0. An RTOS application can run that cooperative
USB host plus all library calls in ONE service task and pass application traffic
through task queues. Do not combine the middleware's separate USBH_USE_OS == 1
thread with unsynchronized library calls from other tasks. That mode, other USB
middleware versions and host FS require their own concurrency/hardware validation.
Timer/radio/other ISR work still posts to the owning task; callbacks stay short.

## 6. Periodic sending and receive callbacks

Hardware timer example: configure a free application timer for a 100 ms update
and add this to your existing HAL callback (do not define a duplicate callback):

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7) { AppBT_RequestTxFromISR(); }
    /* Preserve other timer/timebase handling required by your project. */
}
```

Start that timer with HAL_TIM_Base_Start_IT(&htim7) after initialization. TIM7 is
an example: select a timer not already used as the HAL timebase or by the product.
The ISR only posts a request. AppBT_Process constructs/sends one 10-byte example
packet from main-loop context: ASCII BT, a little-endian 32-bit packet counter,
and a little-endian 32-bit tick value. It retains rejected data exactly for retry.
Replace the example packet construction with your sensor/protocol payload in
main context; Send copies accepted bytes, so its input buffer can then be reused.
Missed timer requests coalesce; this is periodic telemetry, not a lossless sample
queue. Use an application-owned ring buffer if every sample must be retained.

For example, consume the copied RX stream without waiting for a complete message:

```c
uint8_t chunk[64];
uint16_t n = AppBT_Read(chunk, sizeof(chunk));
if (n != 0U) { MyStreamParser_Push(chunk, n); } /* your application parser */
```

Alternatively use a main-loop software timer, without enabling the hardware
request source at the same time:

```c
static uint32_t last_tx_tick; /* declared in the owning function */
uint32_t now = HAL_GetTick();
if ((uint32_t)(now - last_tx_tick) >= 100U) {
    last_tx_tick = now;  /* skip missed periods instead of building a backlog */
    AppBT_RequestTxFromISR(); /* this request-only function is also safe here */
}
```

The example's registered receive callback copies transient bytes into a 256-byte
application ring. Read it regularly from main context with AppBT_Read(buffer,
capacity) and feed your parser. AppBT_RxDrops counts bytes dropped when that
example ring is full; size/drain it for your data rate. This is separate from the
library's TX backpressure. A callback is a byte-span notification, not an interrupt
and not a complete-message event. Disconnect resets this example's parser
buffer; connection changes reset pending telemetry. Early RX preceding the open
notification is retained. Replace that policy when your protocol needs more.

## 7. Saving bonds without hiding flash waits

This is the important 0.1 -> 0.2 API behavior change: BT_Library_Process no longer
saves bonds. Pairing keys work in RAM immediately; persistence requires an explicit
BT_Library_SaveBonds call while disconnected/HCI idle and your application allows
a blocking maintenance window. One call writes at most one pending device.
Inspect BT_Bonds_Dirty to know if more work remains. Example application policy:

```c
/* MAIN CONTEXT ONLY, after polling; never in the timer/RX callback. */
if (maintenance_window_allowed && !BT_Library_IsSerialOpen()) {
    uint8_t all_saved = BT_Library_SaveBonds();
    (void)all_saved; /* 0 can mean pending, unsafe/busy, full or I/O failure. */
}
```

A false serial-open state alone is not proof the entire HCI link is idle; the
bond manager also checks ACL, pairing and command state before writing. Do not
busy-loop until saving succeeds. A new bond is lost on power removal if it was
never committed. BT_Bonds_ClearAll is a separate explicit blocking factory erase;
never invoke it automatically for reconnect problems.

## 8. Acceptance test and troubleshooting

Build, flash without full-chip erase, run freely, pair, connect, and check
BT_SPP_SerialOpen == 1. The echo example should return exact binary bytes; the
PeriodicCallback example sends BT telemetry instead of automatically echoing.
Verify receive bytes through AppBT_Read/AppBT_RxBytes. Repeat disconnect/reconnect,
enter the maintenance window until Dirty clears, power-cycle and verify bond reuse.
Examples/STM32F746G-DISCO/watch.ini retains the familiar diagnostic variables.
Also load Examples/PeriodicCallback/watch.ini to inspect RX bytes/drops and TX
accepted packets. These initialization scripts add watch expressions only.

If SDP/RFCOMM stops, inspect the HCI and SPP trace rings for that same attempt.
If TX returns 0, handle disconnected/full states rather than waiting inside a
callback. If storage fails, check physical flash size, sector boundary, full-sector
size, linker reservation, programmer erases and FlashErrors/Full/Dirty counters.
USB stalls/errors remain visible; automatic recovery is not universally qualified.
For changes to a CubeMX setting, regenerate and verify the resulting header,
include paths, exactly one copy of each source, registration and linker map.
