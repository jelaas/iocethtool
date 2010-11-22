#!/bin/bash

# emulates ethtool -S DEV

PATH=$PATH:.

TMP=/tmp/gstr$$
DEV=${1:-eth0}

tot=$(iocethtool $DEV ETHTOOL_GSTATS d4:)
if [ $? != 0 ]; then
    echo no stats available
    exit 1
fi

n=$tot
while [ $n != 0 ]; do
    fs="$fs s32:"
    fn="$fn d8:"
    n=$[ n-1 ]
done

function out {
    while true; do
	read A B || exit
	read C D
	echo "$D: $B"
    done
}

iocethtool $DEV ETHTOOL_GSTRINGS 4:1 4: $fs|tail -n +3 > $TMP.str
iocethtool $DEV ETHTOOL_GSTATS 4:1 $fn|tail -n +2 > $TMP.stat

(cat -n $TMP.str;cat -n $TMP.stat)|sort -n|out|sort
rm -f $TMP.stat $TMP.str
