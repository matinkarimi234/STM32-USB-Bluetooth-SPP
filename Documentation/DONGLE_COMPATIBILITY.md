# USB Bluetooth dongle compatibility

Scope: STM32F746 USB OTG HS Host, Bluetooth Classic RFCOMM/SPP, baseline
`ba520c3`. This is a project compatibility list, not a list of devices supported
by Windows, Linux, or BTstack. Checked November 13, 2026.

## What is supported today

Only the user's tested CSR-compatible USB dongle, VID:PID `0A12:0001`, has a
reported successful phone/STM32 hardware result with this project. The report
confirms that setup works; it does not qualify every adapter with the same IDs,
every CSR clone, every hardware revision, or long-duration operation.

## Candidate matrix

| Dongle / family | Identification | Project status | Work before listing as supported |
| --- | --- | --- | --- |
| User's existing CSR-compatible adapter | `0A12:0001` | Hardware reported working | Record exact unit, USB revision, and HCI version; complete endurance/reconnect tests |
| Other CSR8510-based adapters | Verify the actual controller; `0A12:0001` alone does not prove identical silicon/firmware | First candidates to test | Run the qualification procedure on each product/revision |
| Ezurio / Laird BT820 | Manufacturer specifies QCA/CSR8510 and USB HCI; record actual USB IDs | Candidate, untested here | Validate descriptors, initialization, security, and SPP. Manufacturer lists BT820 as NRND; consider availability before a new design |
| ASUS USB-BT400 | Known USB identity `0B05:17CB`; BCM20702-family implementations | Candidate, untested here | Check startup requirements; may need a matching Broadcom PatchRAM loader, then qualify |
| Broadcom BCM20702-based USB adapters, including the DeLOCK example in BTstack documentation | Example USB identity `0A5C:21E8` | Candidate, untested here | Identify exact product/revision; assess firmware patches and run qualification |
| TP-Link UB500 units identified as `2357:0604` | Realtek RTL8761BU family for that known identity | Future development target; not currently supported | Implement controller identification, firmware/config loading and startup handling; qualify exact revision |
| ASUS USB-BT500 units identified as `0B05:190E` | Realtek 8761BUV family in Linux USB driver tables | Future development target; not currently supported | Same Realtek startup work, then separate product qualification |
| TP-Link UB400 | Record hardware revision and actual USB IDs first | Revision-dependent; unqualified | Classify the actual unit by controller family. Do not treat the UB400 product name as proof of CSR compatibility |

The candidate ordering is an engineering assessment based on the current code
and controller interfaces, not evidence that an untested adapter already works.
The current firmware has no Broadcom PatchRAM or Realtek firmware/config uploader.
It matches a Bluetooth USB interface rather than a fixed list of VID/PIDs; adding
a USB ID alone cannot implement missing controller initialization.

## Source evidence

* [Ezurio BT820](https://www.ezurio.com/part/bt820) identifies the CSR8510 USB dongle;
  [BT800 family page](https://www.ezurio.com/wireless-modules/bluetooth-modules/bluetooth-42-and-40-modules/bt800-series-bluetooth-module)
  lists its lifecycle status.
* [BTstack's own USB port documentation](https://github.com/bluekitchen/btstack/blob/master/port/libusb/README.md)
  identifies the ASUS BT400 and Broadcom/DeLOCK USB IDs and describes Broadcom
  patch upload and Realtek patch/config initialization. Those are examples for
  BTstack, not compatibility results for this STM32 project.
* [Linux USB Bluetooth driver source](https://code.googlesource.com/linux/torvalds/linux/+/b8c60e8fc6f755c2cdf7164931afdbfa670c6646/drivers/bluetooth/btusb.c)
  groups `2357:0604` and `0B05:190E` with Realtek 8761BUV devices.
  [An upstream UB500 driver patch](https://lore.kernel.org/all/20260824053227.317496-1-junjie.cao@intel.com/)
  identifies the UB500 `2357:0604` / RTL8761BU combination.
* [TP-Link's UB400 download page](https://www.tp-link.com/us/support/download/ub400/)
  has hardware-revision-specific support; verify the unit instead of applying
  one product-name assumption to all revisions.

## Requirements and exclusions

The current application requires a USB HCI controller with Bluetooth Classic
BR/EDR and Secure Simple Pairing. Its expected USB interface is class/subclass/
protocol `E0/01/01`, with HCI commands over control endpoint zero, events over
interrupt IN, and ACL data over bulk IN/OUT. Descriptor matching is necessary
but is not sufficient to prove compatibility. Alternate layouts need driver work.

BLE-only controllers cannot supply this Classic SPP connection. USB audio
transmitters that expose only USB Audio, HID proxy-only receivers, and proprietary
receivers without an accessible Bluetooth HCI interface do not fit this driver.
Bluetooth version printed on a package is not a substitute for controller and
interface identification. This list is for serial communication, not A2DP/HFP
or other audio profiles.

## Identifying an adapter before use

On Windows, inspect Device Manager > Bluetooth adapter > Properties > Details >
Hardware Ids. Example: `USB\VID_0A12&PID_0001`. Record the revision suffix too.
On Linux, `lsusb` reports VID:PID. Record product label, hardware revision,
USB descriptors, and controller version. In Keil, `BT_VersionInfo` exposes HCI
version/revision, manufacturer, and LMP version/subversion.

VID/PID and the advertised CSR name do not uniquely identify genuine silicon.
Use the observed controller versions and actual behavior to qualify samples.

## Promoting a candidate to supported

For each exact product/revision:

1. Power-cycle the MCU and adapter together; record enumeration, descriptors,
   HCI version, and any firmware files/startup sequence needed.
2. Pair using the intended security policy and establish the Android SPP stream.
3. Verify exact binary echo, including 00/80/FF, and at least 64 KiB in small
   chunks (for example 256 bytes, waiting for the matching echo between chunks).
   Do not submit a 64 KiB write to the firmware's bounded send buffer.
4. Perform at least 20 disconnect/reconnect cycles and adapter unplug/replug.
5. Save a bond, power-cycle the MCU, and verify key reuse without re-pairing.
6. Check ACL errors, queue drops, echo drops, and trace history; repeat on Windows
   if Windows is part of the product's supported configuration.
7. Record firmware commit, phone/OS, exact results and limitations in this file.

Suggested current user-facing wording:

> Tested with the project's CSR-compatible 0A12:0001 adapter. Other dongles require
> qualification. Broadcom and Realtek adapters are development candidates, not
> currently verified compatible devices.
