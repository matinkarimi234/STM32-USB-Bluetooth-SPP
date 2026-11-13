# CubeMX package

## 0.2.0 development preview

Build with `./Tools/build-pack.ps1`. The versioned .pack, standalone .pdsc and
SHA256SUMS are written to ignored dist/. Install the .pack through CubeMX's
embedded software package manager, From Local. The public `pack` release still
serves 0.1.0; this branch's 0.2.0 is not that published release.

The enhanced pack exposes USB / ClassicSPP and USB / F746Flash. Its additional
software settings generate STM32Bluetooth/App/bt_user_config.h, including the
reserved paired-device flash address, name, channel, bond count and development
pairing policy. The port derives sector geometry and checks the physical flash
size. CubeMX does not reserve the linker region or wire custom USB registration.
Follow the [usage guide](../Documentation/USAGE_GUIDE.md) for both manual steps,
continuous polling, callbacks, timer requests and explicit flash maintenance.

## Validation on 2026-11-13

- Installed Keil PACK.xsd accepts the PDSC and all declared paths exist.
- 0.2.0 .pack installed through CubeMX's local import/MIT-license flow.
- STM32CubeMX 6.15.0 displayed both components, both activation modes and all
  five settings in an isolated STM32F746NGHx project derived from the example IOC.
- Changing Reserved bond flash address from sector 7 to sector 5 generated
  BT_BONDS_FLASH_START 0x08040000UL in the header, with all five defines correct.
  Regeneration retained that setting. Generated sources and include paths exist.
- After the documented manual registration/polling, example inclusion and linker
  reservation, the generated Keil project built with ARMCC 5.06 update 7:
  zero errors/warnings. The final map limits LR_IROM1 and ER_IROM1 to 0x40000,
  excluding the selected sector. Regeneration restored the built-in HID class
  registration; the manual change must be reviewed/reapplied.
- A separate Keil-only build using the original board middleware plus the new
  library/PeriodicCallback example also completed with zero errors/warnings.
- Seven native suites pass, plus a concrete-port matrix of all eight F746 sectors,
  including 512/1024 KiB physical-flash bounds and failed/torn journal writes.

CubeMX's log contains `Invalid RefComponent list` diagnostics during copying even
though it reports generation success; all required C/header paths were checked
and the resulting application built. Treat clean CubeMX diagnostics and broader
version compatibility as remaining packaging work. No hardware test of the 0.2
changes was performed; the confirmed hardware result belongs to the 0.1 library.

URL installation of the new version, automated custom USB registration, automatic
linker reservation, other CubeMX/toolchain versions, USBH_USE_OS=1 and new
MCU/controller combinations remain unqualified. This is a configurable source
pack with explicit application integration, not a fully generated turnkey driver.

## Distribution and ST status

The public feed uses one `pack` GitHub release containing the current PDSC and all
versioned pack files. The PDSC url points to that stable release asset directory.
When publishing a new version, retain prior versioned pack assets. Do not replace
the public feed with a PDSC that advertises an unavailable version.

ST catalog listing/endorsement and upstream contribution are separate from local
pack distribution. This community project claims neither ST approval nor default
catalog inclusion. The upstream proposal is tracked in
[ST USB Host issue 20](https://github.com/STMicroelectronics/stm32-mw-usb-host/issues/20).

References:

- [ST UM2739: software packs enhanced for STM32CubeMX](https://www.st.com/resource/en/user_manual/um2739-how-to-create-a-software-pack-enhanced-for-stm32cubemx-using-stm32-pack-creator-tool-stmicroelectronics.pdf)
- [ST UM2285: expansion-package development guidelines](https://www.st.com/resource/en/user_manual/um2285-development-guidelines-for-stm32cube-expansion-packages-stmicroelectronics.pdf)
