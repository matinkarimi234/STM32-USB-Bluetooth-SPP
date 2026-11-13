# STM32 USB Bluetooth Classic SPP

MIT-licensed USB Bluetooth Host class and Classic RFCOMM/SPP source library for
STM32 projects using ST USB Host middleware. The 0.1.0 library layout was reported
working on STM32F746G-DISCO by the project owner.

**Start here: [CubeMX and Keil usage guide](Documentation/USAGE_GUIDE.md).**
It covers project setup, flash reservation, timer-triggered TX, receive callbacks,
backpressure, pairing-key maintenance, migration and hardware acceptance tests.

## Current development: 0.2.0 preview

This branch adds a CubeMX configuration page and generated project header, including
the reserved paired-device flash address. The F746 port derives the erase sector
and size from that address and checks the physical flash-size fuse. You must still
reserve that entire sector in the linker and preserve it during firmware updates.

Normal serial operation is cooperative and nonblocking: poll USBH_Process and
BT_Library_Process in one owning main loop/task. BT_Library_Send copies an entire
span into its bounded queue and returns 1 (accepted) or 0 (closed/invalid/full).
Received bytes arrive through a transient-span callback in that same owning
context. No transport API should be called from a timer ISR.

[PeriodicCallback](Examples/PeriodicCallback/app_bt_periodic.c) demonstrates an ISR
posting TX requests, main-context sending with exact retry on backpressure, and an
RX callback copying bytes into a bounded application ring. It coalesces timer
requests; it is not a lossless sampling queue. The separate echo application is
in Examples/STM32F746G-DISCO.

**Migration from 0.1:** BT_Library_Process no longer writes flash. New pairing keys
remain in RAM until the application explicitly calls BT_Library_SaveBonds during
a disconnected, noncritical maintenance window. Internal HAL flash program/erase
is blocking. See the guide before upgrading, or new bonds will not survive reset.

## Use in a project

Compile all Src/*.c and exactly one storage/timing port. Add Inc/ and your project
configuration directory to include paths. For manual integration, copy
Templates/bt_user_config.h into your project; CubeMX generates that header from
its parameters. No BT_USE_USER_CONFIG compiler define is required in 0.2.

Supply STM32 HAL/HCD, CMSIS and ST USB Host middleware from your project. Register
once after USBH_Init and before USBH_Start. Poll continuously and keep callbacks
short. The preview does not automatically wire that application-specific USB
registration order. Follow the complete guide for the exact calls and linker
settings, including regeneration checks.

One host/controller/ACL/SPP service is supported. Name is 1..64 bytes, server
channel 1..30, and bond capacity 1..16. Development Just Works pairing is disabled
by default; enable it deliberately for a controlled initial test. A product with
user-approved pairing needs its own policy.

The hardware-tested Keil/MAUI application remains separately maintained:
[STM32F746 Bluetooth example](https://github.com/matinkarimi234/Stm32f746_Bluetooth_Test).
This branch does not alter that firmware repository.

## CubeMX package

Build with `./Tools/build-pack.ps1`, then import
`dist/MatinKarimi.STM32Bluetooth.0.2.0.pack` using the local package manager.
The [published source-pack preview](https://github.com/matinkarimi234/STM32-USB-Bluetooth-SPP/releases/tag/pack)
currently contains 0.1.0 and does not include the new settings. See
[package validation status](Packaging/README.md) before distributing a build.
This is community software; ST catalog inclusion and upstream acceptance are
separate processes. No ST approval is claimed.

## Hardware and validation

Only the owner's exact CSR-compatible 0A12:0001 dongle is hardware confirmed here.
Other CSR, Broadcom or Realtek adapters need qualification and possibly vendor
startup support. See [dongle compatibility](Documentation/DONGLE_COMPATIBILITY.md).
BLE-only controllers cannot provide Classic SPP. No audio profile is implemented.
The 0.2 changes still need the hardware acceptance cycle in the guide.

On Windows with Zig installed, run:

```powershell
./Tests/run_tests.ps1 -Zig <path-to-zig.exe>
```

Seven suites compile production C with USB/flash mocks, including the periodic
callback example, followed by an eight-sector F746 geometry matrix. These tests
complement physical USB/radio testing. No Python dependency or source is required.

Issues and pull requests are welcome. Include board, dongle VID/PID/revision,
controller HCI/LMP versions, toolchain, phone/OS, reproduction steps and a trace
with key material removed. Run the tests and report hardware results.
Contributions are under MIT; preserve third-party notices. ST HAL/USB middleware
is a dependency supplied by the consuming STM32Cube project, not included here.
