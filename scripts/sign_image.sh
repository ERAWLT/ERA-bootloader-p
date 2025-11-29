#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

COMMIT_HASH=$(git rev-parse --short HEAD)
DATE=$(date +"%Y%m%d")

if [ -n "$2" ]
then
    FW_SECURITY_CTR=$2
else
    FW_SECURITY_CTR=1
fi
echo "input FW_SECURITY_CTR (major):" $FW_SECURITY_CTR

if [ -n "$3" ]
then
    # packed: maj.min.rev+build
    VERSION=$FW_SECURITY_CTR.$3
else
    VERSION=1.2
fi
echo input minor.build: $VERSION

if [ -n "$4" ]
then
    KEY=$4
else
    KEY=$SCRIPT_DIR/contrib/key_ecdsa-p256.pem
fi
echo input key: $KEY

# the result file:

if [ -n "$1" ]
then
    INPUT_BINARY=$1/era-bootloader.bin
    OUTPUT_BINARY=$1/era-bootloader_signed.bin
    USER_BINARY_NAME=$1/BL_${VERSION}_dev_signed_${COMMIT_HASH}.bin
else
    INPUT_BINARY=$SCRIPT_DIR/../src/era-bootloader.bin
    OUTPUT_BINARY=$SCRIPT_DIR/../src/era-bootloader_signed.bin
    USER_BINARY_NAME=$SCRIPT_DIR/../src/BL_${VERSION}_dev_signed_${COMMIT_HASH}.bin
fi
echo input binary file: $INPUT_BINARY

IMGTOOL=$SCRIPT_DIR/../mcuboot/scripts/imgtool.py
LOAD_ADDR=0x8020000
SLOT_SIZE=0x20000
HEADER_SIZE=0x400

# Firmware descriptor and security counter values
FW_DESCRIPTOR=BOOTLOADER

# Dependencies for the bootloader
DEP_VERSION_OF_MF=0.10.152
DEP_MF_IMAGE_ID=0
DEP="($DEP_MF_IMAGE_ID,$DEP_VERSION_OF_MF)"

$IMGTOOL sign --key $KEY -H $HEADER_SIZE -S $SLOT_SIZE -L $LOAD_ADDR -M 2 --p1363 --custom-tlv 0xA3 $FW_DESCRIPTOR --security-counter $FW_SECURITY_CTR --align 32 --max-align 32 --pad-header -v $VERSION -d $DEP  $INPUT_BINARY $OUTPUT_BINARY
# $IMGTOOL sign --key $KEY -H $HEADER_SIZE -S $SLOT_SIZE -L $LOAD_ADDR -M 2 --p1363 --custom-tlv 0xA3 $FW_DESCRIPTOR --security-counter $FW_SECURITY_CTR --align 32 --max-align 32 --pad-header -v $VERSION  $INPUT_BINARY $OUTPUT_BINARY

# check status of execution
if [ $? -ne 0 ]; then
  echo "Error while signing"
  exit 1
else
  $IMGTOOL dumpinfo $OUTPUT_BINARY
  cp $OUTPUT_BINARY $USER_BINARY_NAME
  echo "result in: $USER_BINARY_NAME"
fi
exit 0
