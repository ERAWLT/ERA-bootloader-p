# ERA-Bootloader 🛡️     
This firmware is ERA-wallet's bootloader based on [MCUboot](https://github.com/mcu-tools/mcuboot), a secure bootloader for 32-bit microcontrollers.

> ⚠️ **Open Source Status Notice**  
> While ERA is committed to full open-source transparency, this repository is currently only partially open. Certain proprietary components remain private due to ongoing refactoring, audits and pending patent applications. **As a result, this code cannot be compiled or built independently.** The published source code is provided for reference and review purposes only. We are actively working toward making all code fully open source once these processes are complete.

## 📂 Repository Structure
| Directory      | Description |
| ----------- | ----------------- |
| *[doc](doc/)*     | 📄 Firmware documentation |
| *[src](src/)*     | 📦 ERA-wallet's Bootloader sources |
| *[scripts](scripts/)* | 🛠️ Bootloader scripts to build and sign firmware, also including test key |
| *[mcuboot](https://github.com/ERAWLT/mcuboot)* | 🔗 ERA-wallet's fork from the original [MCUboot](https://github.com/mcu-tools/mcuboot) repository where verification using the secure element was added |
| *[ERA-framework](https://github.com/ERAWLT/ERA-framework)* | 🧩 ERA-wallet's framework |
| *[.vscode](.vscode/)*| 🖥️ [Visual Studio Code](https://code.visualstudio.com) settings for developers |
| *[.github](.github/)*| ⚙️ CI files |
| *[.docker](.docker/)*| 🐳 Docker files |

## 🔧 General
This repository contains an advanced bootloader based on the robust and widely used [MCUboot](https://github.com/ERAWLT/mcuboot). The bootloader utilizes FreeRTOS, a real-time operating system for microcontrollers, to perform its operations efficiently and reliably.

## ✨ Key Features
The bootloader in this repository has been enhanced with additional features to provide more robust security and flexibility for firmware updates.

- **🔒 Firmware Verification**:  
  The bootloader has the ability to verify the main firmware using a secure element. This feature ensures the integrity and authenticity of the firmware before it is executed, protecting against tampering.

- **📤 Firmware Update**:  
  The bootloader supports downloading and installing updates. If the downloaded update passes the security checks, it can overwrite the previous version without requiring a backup. This ensures the device remains up-to-date even in cases of critical firmware malfunctions.

- **🛡️ Security Checks**:  
  Security checks include a version counter (provided by MCUboot). If the update version is lower than the current one, the update will be canceled to prevent downgrade attacks.

- **🚀 Bootloader Update**:  
  The bootloader can be updated via the [ERA-bootstrapper](https://github.com/ERAWLT/ERA-bootstrapper). Update mechanisms are identical to those for the main firmware, allowing fixes for critical bootloader errors if needed.

- **📦 Shipping Mode**:  
  The bootloader supports a shipping mode to conserve battery power during storage and transportation. To exit this mode, connect wireless charging.

## 🔑 Key Protection
The public key for verifying firmware signatures is securely stored in the ATECC608C secure element.

The **ATECC608C** is a CryptoAuthentication device from Microchip Technology. It provides robust hardware-based security for applications like secure boot, secure downloads, and secure communications.

Key features of the ATECC608C include:
- 🔐 **EEPROM Storage**: Securely stores up to 16 keys, certificates, passwords, or secret data.
- 🔢 **Monotonic Counter**: Prevents brute-force attacks by limiting password attempts.
- 🔒 **Symmetric Key Protection (IOP)**: Each device has a unique 32-byte symmetric key (IOP) configured at manufacturing. The IOP never leaves the secure element or MCU memory, ensuring secure exchanges.

The bootloader leverages these features to ensure firmware authenticity and protect against unauthorized access.

## 📖 Firmware Documentation
- 📚 [How to build and configure the project](doc/build_helper.md)
- 🛠️ [Getting started with OpenOCD](doc/openocd_helper.md)
- 🔧 [MCUboot configuration options](src/mcuboot/Inc/mcuboot_config/mcuboot_config.h)
- 📋 [Flash memory layout for MCUboot](src/mcuboot/Inc/sysflash/sysflash.h)

## 📚 Used Libraries:
- 🔗 [MCUboot](https://github.com/mcu-tools/mcuboot)
- 🔒 [Mbed TLS](https://github.com/Mbed-TLS/mbedtls)
- 🛡️ [Microchip CryptoAuthentication Library](https://github.com/MicrochipTech/cryptoauthlib)

## 🤝 Contributing
We welcome contributions to this bootloader repository. If you have a feature request, bug report, or wish to contribute code, please feel free to open an issue or submit a pull request.

## 📜 License
This ERA-Bootloader is open-source software, licensed under the terms provided in the [LICENSE](./LICENSE) file in the repository. Please review this before using or modifying the bootloader.

## 📧 Contact
If you have any questions or concerns about this bootloader, please contact the repository maintainers through the repository's issue tracker.
