#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# check once - signature file below!
INPUT_FIX_SIG=$SCRIPT_DIR/contrib/bootloader_signature_base64.bin

# change once constants for any build:
IMGTOOL=$SCRIPT_DIR/../mcuboot/scripts/imgtool.py
LOAD_ADDR=0x8020000
SLOT_SIZE=0x20000
HEADER_SIZE=0x400
FW_DESCRIPTOR=BOOTLOADER

# changable main path below:
INPUT_BINARY=$SCRIPT_DIR/../build-release-h7v3/src/era-bootloader.bin

# changable parts below:
FW_SECURITY_CTR=0

# get version of unsigned file by offset
VERSION_OFFSET=682
VERSION=$(dd if="$INPUT_BINARY" bs=1 skip=$VERSION_OFFSET count=100 2>/dev/null | strings | head -n 1)

COMMIT_HASH=$(git rev-parse --short HEAD)
DATE=$(date +"%Y%m%d")

# the result file:
OUTPUT_BINARY=$SCRIPT_DIR/BL_${VERSION}_prod_signed_${DATE}_${COMMIT_HASH}.bin

# Dependencies for the bootloader
DEP_VERSION_OF_MF=0.10.152
DEP_MF_IMAGE_ID=0
DEP="($DEP_MF_IMAGE_ID,$DEP_VERSION_OF_MF)"

$IMGTOOL sign --fix-sig $INPUT_FIX_SIG -H $HEADER_SIZE -S $SLOT_SIZE -L $LOAD_ADDR -M 2 --p1363 --custom-tlv 0xA3 $FW_DESCRIPTOR --security-counter $FW_SECURITY_CTR --align 32 --max-align 32 --pad-header -v $VERSION -d $DEP $INPUT_BINARY $OUTPUT_BINARY
# $IMGTOOL sign -H $HEADER_SIZE -S $SLOT_SIZE --custom-tlv 0xA3 $FW_DESCRIPTOR --security-counter $FW_SECURITY_CTR --align 32 --max-align 32 --pad-header -v $VERSION  $INPUT_BINARY $OUTPUT_BINARY

# check status of execution
if [ $? -ne 0 ]; then
  echo "Error while signing"
  exit 1
else
  $IMGTOOL dumpinfo $OUTPUT_BINARY
  echo "result in: $OUTPUT_BINARY"
fi

# uncomment to verify
# KEY=$SCRIPT_DIR/contrib/firmware_key_p256.pem
# $IMGTOOL verify $OUTPUT_BINARY -k $KEY

exit 0
