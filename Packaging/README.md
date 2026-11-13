# CubeMX package

This initial CMSIS-Pack exposes USB / Bluetooth components for the Classic SPP
core and the STM32F746NGHx sector-7 port. It requires ST USB Host/HAL from the
consuming project and manual registration/polling/configuration as in README.
It has no enhanced CubeMX configuration or automatic integration templates yet.

Build with `./Tools/build-pack.ps1`. The .pack and standalone .pdsc are written
to dist/. Local install is through CubeMX's embedded software package manager,
From Local. URL installation uses the .pdsc release asset after publication.

The public feed is intended to use one `pack` GitHub release containing the
current PDSC and all versioned pack files. PDSC <url> points to that stable release
asset directory. Updating a version should retain previous versioned .pack assets.

ST catalog listing/endorsement is separate from local/URL pack distribution. We
must satisfy ST's expansion-package requirements and work through its ecosystem/
partner process before claiming an ST catalog listing. This project is community
software and does not claim ST approval or automatic catalog inclusion.

Validation status will be recorded after schema and installed-tool checks.
Do not describe basic source-component installation as fully generated CubeMX
integration. The next milestone is an enhanced package with generated callbacks,
configurations, correctly ordered USBH_Init/Register/Start calls and regeneration
validation on a clean project.

References:
- https://www.st.com/resource/en/user_manual/um2739-how-to-create-a-software-pack-enhanced-for-stm32cubemx-using-stm32-pack-creator-tool-stmicroelectronics.pdf
- https://www.st.com/resource/en/user_manual/um2285-development-guidelines-for-stm32cube-expansion-packages-stmicroelectronics.pdf
