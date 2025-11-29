#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"


if [ -n "$1" ]
then
    NEW_KEY=$1
else
    NEW_KEY=$SCRIPT_DIR/contrib/key_ecdsa-p256.pem
fi
echo "key file: $NEW_KEY"


IMGTOOL=$SCRIPT_DIR/../mcuboot/scripts/imgtool.py
EXPORT=$SCRIPT_DIR/../mcuboot/scripts/export_keys.py

$IMGTOOL keygen -k $NEW_KEY -t ecdsa-p256
python3 $EXPORT -o console -k $NEW_KEY