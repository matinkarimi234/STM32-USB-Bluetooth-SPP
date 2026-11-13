# STM32 USB Bluetooth Classic SPP

MIT-licensed USB Bluetooth Host class and Classic RFCOMM/SPP source library for
STM32 projects using ST USB Host middleware. The STM32F746G-DISCO integration is
reported working by the project owner, including the extracted library layout.

## CubeMX status

The first package is a CMSIS source component intended for local/URL installation
in STM32CubeMX. Automatic CubeMX code generation and ST catalog inclusion are
separate milestones. This is community-maintained software, not an ST-authored or
ST-approved driver. See Packaging/README.md for exact package validation status.

## Integration

Compile every Src/*.c and one storage/timing port. Add Inc/ to include paths,
copy Templates/bt_user_config.h into your project, and define BT_USE_USER_CONFIG
for every library translation unit. The supplied port supports the 1 MB F746
sector-7 layout only: reserve 0x080C0000..0x080FFFFF in the linker and avoid erasing
that sector during firmware updates. Other layouts need a different bt_port.h
implementation. The on-flash bond record format is unchanged.

Call USBH_Init, then BT_Library_Register(host, callbacks), then USBH_Start.
Poll USBH_Process(host) followed by BT_Library_Process() from the same main loop.
Use BT_Library_IsSerialOpen(), BT_Library_Send() and the received/serial_changed
callbacks. Callback spans are transient; consume/copy them immediately and do not
block polling. Send returns 1 for an atomic enqueue and 0 for disconnected,
invalid or full. Handle backpressure; pending serial buffer capacity is 1024 bytes.

Echo is application policy; see Examples/STM32F746G-DISCO/app_bluetooth.c.
Generic defaults disable automatic development pairing. The example explicitly
enables the existing development Just Works policy. Name is 1..64 bytes, server
channel 1..30, bond capacity 1..16. One host/controller/ACL/SPP service is supported.

The full working Keil/MAUI example is maintained separately:
https://github.com/matinkarimi234/Stm32f746_Bluetooth_Test

## Hardware

Only the owner's exact CSR-compatible 0A12:0001 dongle is hardware confirmed here.
Other CSR, Broadcom, or Realtek adapters need qualification and possibly vendor
startup support. See Documentation/DONGLE_COMPATIBILITY.md. BLE-only controllers
cannot provide Classic SPP. No audio profile support is claimed.

## Tests and contribution

Install Zig and run `./Tests/run_tests.ps1 -Zig <path-to-zig.exe>` on Windows.
There is no Python source/dependency. Six suites compile production C with USB/
flash mocks. These complement, but do not replace, physical USB/radio testing.

Issues and pull requests are welcome. Include exact board, dongle VID/PID/revision,
controller HCI/LMP versions, toolchain, phone/OS, reproduction steps, and a trace
with key material removed. Run the tests and report hardware results for changes.
Contributions are under MIT; preserve third-party notices. ST HAL/USB middleware
is a dependency supplied by the consuming STM32Cube project, not included here.
