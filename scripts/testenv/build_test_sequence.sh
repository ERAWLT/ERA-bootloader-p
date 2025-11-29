#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SCRIPT_DIR=$SCRIPT_DIR/..

if [ -n "$1" ]
then
    BOARD_REV=$1
else
    BOARD_REV=h7v2
fi

BUILD_TYPE=Release

CORRUPT_PY=$SCRIPT_DIR/testenv/corrupt_fw.py

BUILD_DIR=$SCRIPT_DIR/../release$BOARD_REV
mkdir $BUILD_DIR
cd $BUILD_DIR
rm -rf testseq
mkdir testseq

cp $SCRIPT_DIR/../version_project.h $SCRIPT_DIR/../version_project.back

KEY0=$SCRIPT_DIR/contrib/key_ecdsa-p256.pem
KEY1=$BUILD_DIR/testseq/BL_key1_ecdsa-p256.pem
KEY2=$BUILD_DIR/testseq/BL_key2_ecdsa-p256.pem
KEY3=$BUILD_DIR/testseq/BL_key3_ecdsa-p256.pem
$SCRIPT_DIR/gen_key.sh $KEY1
$SCRIPT_DIR/gen_key.sh $KEY2
$SCRIPT_DIR/gen_key.sh $KEY3

OUTPUT_BIN=./src/era-bootloader_signed.bin

# -----------------
# usually seq below
# -----------------

SEC_CNT=0
BASE=7
BUILD=10
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    
    $SCRIPT_DIR/testenv/new_key_bad_inside.sh ./src $SEC_CNT $BASE.$BUILD $KEY1 $KEY0
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey1_STILL_OLD_INSIDE_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

    mkdir ./testseq/BL_by_key0_ok_CORRUPT
    cp $( printf './testseq/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) ./testseq/BL_by_key0_ok_CORRUPT
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "header minor"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "header magic"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "header img_size"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "signature"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "trailerInfo"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "trailerInfoProt"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "less"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_by_key0_ok_CORRUPT/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "more"

SEC_CNT=0
BASE=7
BUILD=10
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY1
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key1_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY2
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key2_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY3
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key3_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

SEC_CNT=0
BASE=7
BUILD=11
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY1
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key1_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY2
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key2_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY3
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key3_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )


SEC_CNT=1
BASE=7
BUILD=12
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY1
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key1_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY2
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key2_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )
    $SCRIPT_DIR/sign_image.sh ./src $SEC_CNT $BASE.$BUILD $KEY3
    mv $OUTPUT_BIN $( printf './testseq/BL_by_key3_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

# -----------------
# new key seq below
# -----------------

SEC_CNT=0
BASE=7
BUILD=10
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY0 $KEY3
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey0_by_key3_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

SEC_CNT=1
BASE=7
BUILD=10
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY1 $KEY0
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY3 $KEY2
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey3_by_key2_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

    mkdir ./testseq/BL_newkey1_by_key0_ok_CORRUPT
    cp $( printf './testseq/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) ./testseq/BL_newkey1_by_key0_ok_CORRUPT/
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "header minor"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "header magic"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "header img_size"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "signature"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "trailerInfo"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "trailerInfoProt"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "wHeader minor"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "wHeader magic"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "wHeader img_size"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "wTrailerInfo"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "wSignature"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "less"
    python3 $CORRUPT_PY -i $( printf './testseq/BL_newkey1_by_key0_ok_CORRUPT/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD ) -t "more"

SEC_CNT=1
BASE=7
BUILD=11
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY2 $KEY1
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey2_by_key1_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY1 $KEY0
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

SEC_CNT=1
BASE=7
BUILD=12
$SCRIPT_DIR/testenv/change_version.sh $SCRIPT_DIR/../version_project.h $SEC_CNT $BASE $BUILD
cmake -DHWLT_BOARD_REVISION=$BOARD_REV -DCMAKE_BUILD_TYPE=$BUILD_TYPE  -B. -S.. --fresh && make -j8
    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY2 $KEY1
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey2_by_key1_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )

    $SCRIPT_DIR/sign_image_new_key.sh ./src $SEC_CNT $BASE.$BUILD $KEY1 $KEY0
    mv $OUTPUT_BIN $( printf './testseq/BL_newkey1_by_key0_ok_s%2.2d_v%2.2F.bin' $SEC_CNT $BASE.$BUILD )


rm -f $SCRIPT_DIR/../version_project.h
cp $SCRIPT_DIR/../version_project.back $SCRIPT_DIR/../version_project.h
rm -f $SCRIPT_DIR/../version_project.back
