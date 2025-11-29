#!/bin/bash
#
# This script increment decimal number next to DEFINE_NAME,
# and do nothing with DEFINE_NAME if there is no any nubmer after.
#
# Script works correct with any tabs or whitespaces afrer DEFINE_NAME
# also if number is DEC and looks like (21) or (21U) - it will be incremented.

#file as arg
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
SCRIPT_DIR=$SCRIPT_DIR/..

if [ -n "$1" ]
then
    INPUT_FILE=$1
else
    INPUT_FILE=$SCRIPT_DIR/../version_project.h
fi
echo input VERSION file: $INPUT_FILE

if [ -n "$2" ]
then
    H3=$2
else
    H3=1
fi

if [ -n "$3" ]
then
    H2=$3
else
    H2=2
fi

if [ -n "$4" ]
then
    H1=$4
else
    H1=3
fi


MAJOR="_BASE"
MINOR="BUILDNUM"

# find number next to $DEFINE_NAME
UPDATED_CONTENT=$(awk '/[a-zA-Z_]*'$MINOR'/ { match($0, /[0-9]+/); num = substr($0, RSTART, RLENGTH); num='$H1'; sub(/[0-9]+/, num); } 1' "$INPUT_FILE")
# Update file
echo "$UPDATED_CONTENT" > "$INPUT_FILE"

# find number next to $DEFINE_NAME
UPDATED_CONTENT=$(awk '/[a-zA-Z_]*'$MAJOR'/ { match($0, /[0-9]+.[0-9]+/); sub(/[0-9]+.[0-9]+/, '$H3'.'$H2'); } 1' "$INPUT_FILE")
# Update file
echo "$UPDATED_CONTENT" > "$INPUT_FILE"
