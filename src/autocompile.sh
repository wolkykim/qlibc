#!/bin/bash

# Copyright (c) 2010-2014 Seungyoung Kim.
# All rights reserved.

ARGC=$#
ARGV=$*

if [ $ARGC -le 0 ]; then
    echo "Usages: $0 monitoring_folders"
    echo "  ex) $0 ./ ./containers/"
    echo "  ex) $0 ./ ./*/"
    echo ""
    echo "In 3 seconds, '$0 ./ ../include/ ./*/' will be executed. Press CTRL+C to stop."
    sleep 3
    $0 ./ ../include/ ./*/
    exit
fi

declare DIR[$ARGC]
declare LS[$ARGC]
declare TITLE="__________oo!oo__[qLibc Auto Compiler]__oo!oo_________[m:compile, r:recompile]_"
declare SPACE="                                                                                "
declare LINE="--------------------------------------------------------------------------------"


print_msg() {
    echo -ne "[qLibc]==> $1"
}

run_cmd() {
    echo -ne "${LINE}\n"
    $1
    echo -ne "${LINE}\n"
    if [ "$?" -eq "0" ]; then
        print_msg "SUCCESS - $1\n"
    else
        print_msg "FAILED - $1\n"
    fi
}

#
# main
#
for ((i = 0; i < $ARGC; i++)); do
    if [ ! -d "$1" ]; then
        echo "$1 is not directory."
        exit
    fi
    DIR[${i}]=$1
    LS[${i}]=`ls -a --full-time ${DIR[${i}]}/*.c ${DIR[${i}]}/*.h Makefile`;
    shift
done

clear
print_msg "Watching ${ARGV} ('m':compile, 'r':recompile)\r"

for ((loop=0; ; loop++)); do
    for ((i = 0; i < $ARGC; i++)); do
        WATCHLIST="${DIR[${i}]}/*.[c,h] Makefile"
        LS_NOW=`ls -a --full-time ${WATCHLIST}`;
        if [ "$LS_NOW" != "${LS[${i}]}" ]; then
            print_msg "File change detected in '${DIR[${i}]}' at `date`\n"
            run_cmd "make"
            print_msg "Watching ${ARGV} ('m':compile, 'r':recompile) - ${loop}\r"
        fi
        LS[${i}]=$LS_NOW
    done

    if read -t 1 -s KEYIN; then
        if [ "$KEYIN" == "r" ]; then
            print_msg "Force to recompile at `date`\n"
            run_cmd "make clean"
            run_cmd "make all"
            print_msg "Watching ${ARGV} ('m':compile, 'r':recompile) - ${loop}\r"
        elif [ "$KEYIN" == "m" ]; then
            print_msg "Force to compile at `date`\n"
            run_cmd "make"
            print_msg "Watching ${ARGV} ('m':compile, 'r':recompile) - ${loop}\r"
        else
            print_msg "UNKNOWN COMMAND : ${KEYIN}\n"
            print_msg "Watching ${ARGV} ('m':compile, 'r':recompile) - ${loop}\r"
        fi
    fi
done
