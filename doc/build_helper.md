# How to build
To use the bootloader, clone the repository and follow the instructions.

```
git clone git@github.com:ERAWLT/hwlt-mcuboot.git --recurse-submodules
```
## Requirements

### Mcuboot
To install requirements for imgtools:
```
scripts/install.sh
```

### Private keys
For generating new private key:
```
scripts/gen_key.sh
```

### Build manager

The build process relies on cmake, a cross-platform open-source build system generator, to manage the build process in a compiler-independent and OS-independent manner.
<details> <summary> <h3> Installing CMake </h3> </summary>

```
sudo apt-get install cmake
```
</details>


### Compiler

The repository uses ARM toolchain 12.3.1 to build the source code. This compiler is designed to generate optimized code for ARM processors.

<details> <summary> <h3> Installing ARM toolchain 12.3.1 </h3> </summary>

Our firmware can only be built with ARM toolchain 12.3.1. If the package manager of your distro has any limitations on selecting required version of the toolchain, you can install it manually

``` 
mkdir arm-toolchain
cd arm-toolchain

# Getting the toolchain archive
wget https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz

# Unpack
tar -xJf arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz 

# Link LTO plugin to correct path
cd arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/
ln -s -v ../../$(find * -type f -name liblto_plugin.so) lib/bfd-plugins/liblto_plugin.so

# Create symlinks (it overrides your arm-none-eabi-* toolchain symlinks if exists)
sudo ln -sf $(pwd)/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-* /usr/bin
``` 
</details>


## Build project

Using cmake presets (you can [change cacheVariables](../CMakePresets.json) in config):
```
cmake --preset release && cmake --build --preset release -j 8
```

Manual:
```
cmake --fresh  -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j 8
```

## Flags for alternate building

### CMAKE_BUILD_TYPE

We have two main build types <b>Release</b> and <b>Debug</b>.
* Debug target has shell/uart and delays during startup for capture shell command from testing scripts
```shell
-DCMAKE_BUILD_TYPE=Debug
```
* Release target has no additional delays for shell, also shell and uart disabled. The main feature of the Release build is that the secure element is used by default. i.e. IOP is used for the exchange. Signature verification is performed via secure element.
```shell
-DCMAKE_BUILD_TYPE=Release
```

### HWLT_BOARD_REVISION
The macro <b> HWLT_BOARD_REVISION_NUM </b> corresponding to the board version (1 as default) will be available in code part c.
To select a specific board revision, use:
```shell
-DHWLT_BOARD_REVISION="h7v1"
-DHWLT_BOARD_REVISION="h7v2"
-DHWLT_BOARD_REVISION="h7v3" # actual revision number
```

### LIBSE
To enable the use of the hardware functions of the secure element (as the default configuration)
```shell
-DUSE_HW_SE=ON
-DHW_SE_IS_NOT_LOCKED=OFF
```

Using a developer board with SE chips (as default OFF):
```shell
-DSE_DEV_BOARD=ON
```

SE can be not locked for debug PCBs. This config will use only API from SE which available for non locked SE. Signature verification is still performed via secure element, but without IOP usage.
```shell
-DUSE_HW_SE=ON
-DHW_SE_IS_NOT_LOCKED=ON
```

To disable hardware and use the soft functions via MbedTLS, for test boards without SE chip. [MbedTLS](https://github.com/Mbed-TLS/mbedtls) will be used to verify the signature.
```shell
-DUSE_HW_SE=OFF
```

### FRAMEWORK
A detailed description of the flags is placed in subproject:
[hwlt-framework](https://github.com/ERAWLT/hwlt-framework/tree/develop)


# Test with bootstrapper
Algorithm to test bootloader firmware with libse:
- [build ERA-Bootloader](#-1-How-to-build) release version
- [build ERA-Bootstrapper](https://github.com/ERAWLT/ERA-bootstrapper) release version
- flash bootloader (below example vscode task commands):
```
"openocd -f ${workspaceFolder}/.vscode/openocd_stm32h7.cfg -c \"program release/src/era-bootloader_signed.bin verify 0x08040000\" -c \"reset halt\" -c shutdown"
"openocd -f ${workspaceFolder}/.vscode/openocd_stm32h7.cfg -c \"program release/src/era-bootloader_signed.bin verify 0x08020000\" -c \"reset halt\" -c shutdown"
```
- flash bootstrapper (below example vscode task command):
```
"openocd -f ${workspaceFolder}/.vscode/openocd_stm32h7.cfg -c \"program release/src/era-bootstrapper.bin verify 0x08000000 reset exit\"
```
- now BS-BL chain successfully loaded to device.

# Sign image script usage
To sign image with default key (check current $KEY file):
```
scripts/sign_image.sh ${CMAKE_BINARY_DIR}/src ${MajorVersion} ${MinorVersion} $KEY
```

To sign image and share new public key inside
(set $OLD_KEY with path to current key file, and $NEW_KEY with path to new key file):
```
scripts/sign_image_new_key.sh ${CMAKE_BINARY_DIR}/src ${MajorVersion} ${MinorVersion} $NEW_KEY $OLD_KEY
```

Default key ($KEY) should be stored in:
- scripts/contrib/key_ecdsa-p256.pem

Default key ($NEW_KEY) should be stored in:
- scripts/contrib/mcuboot_key_ecdsa-p256.pem
