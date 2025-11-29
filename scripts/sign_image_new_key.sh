#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ -n "$1" ]
then
    INPUT_BINARY=$1/era-bootloader.bin
    OUTPUT_BINARY=$1/era-bootloader_signed.bin
else
    INPUT_BINARY=$SCRIPT_DIR/../build/src/era-bootloader.bin
    OUTPUT_BINARY=$SCRIPT_DIR/../build/src/era-bootloader_signed.bin
fi
echo input binary file: $INPUT_BINARY

if [ -n "$2" ]
then
    FW_SECURITY_CTR=$2
else
    FW_SECURITY_CTR=1
fi
echo "input sec counter (major):" $FW_SECURITY_CTR

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
    NEW_KEY=$4
else
    NEW_KEY=$SCRIPT_DIR/contrib/mcuboot_key_ecdsa-p256.pem
fi
echo input new key: $NEW_KEY

if [ -n "$5" ]
then
    OLD_KEY=$5
else
    OLD_KEY=$SCRIPT_DIR/contrib/key_ecdsa-p256.pem
fi
echo input old key: $OLD_KEY

IMGTOOL=$SCRIPT_DIR/../mcuboot/scripts/imgtool.py
LOAD_ADDR=0x8020000
SLOT_SIZE=0x20000
HEADER_SIZE=0x400
WRAPPER_HEADER_SIZE=0x400

# Firmware descriptor and security counter values
FW_DESCRIPTOR=BOOTLOADER

# Dependencies for the bootloader
DEP_VERSION_OF_MF=0.10.152
DEP_MF_IMAGE_ID=0
DEP="($DEP_MF_IMAGE_ID,$DEP_VERSION_OF_MF)"

# use to swap: --align 32 --max-align 32 --pad
printf '\n\n%s\n' 'image #1 - signed with new key.'
$IMGTOOL sign --key $NEW_KEY -H $HEADER_SIZE -S $SLOT_SIZE -L $LOAD_ADDR -M 2   --p1363 --custom-tlv 0xA3 $FW_DESCRIPTOR --security-counter $FW_SECURITY_CTR --align 32 --max-align 32 --pad-header -v $VERSION -d $DEP $INPUT_BINARY $OUTPUT_BINARY
$IMGTOOL dumpinfo $OUTPUT_BINARY

# check status of execution
if [ $? -ne 0 ]; then
  echo "Error while signing"
  exit 1
else
  $IMGTOOL dumpinfo $OUTPUT_BINARY
  echo "result in: $OUTPUT_BINARY"
fi

printf '\n\n%s\n' 'Wrapper (image #2) - signed with old key.'
result=$($IMGTOOL sign --key $OLD_KEY --custom-tlv 0xA2 $NEW_KEY -H $WRAPPER_HEADER_SIZE -S $SLOT_SIZE -L $LOAD_ADDR -M 2  --p1363 --custom-tlv 0xA3 $FW_DESCRIPTOR --security-counter $FW_SECURITY_CTR --align 32 --max-align 32 --pad-header -v $VERSION -d $DEP $OUTPUT_BINARY $OUTPUT_BINARY | grep Error)
if [ "$result" ]
then
    echo $result
else
    $IMGTOOL dumpinfo $OUTPUT_BINARY
fi
echo result in: $OUTPUT_BINARY

exit 0
