#!/bin/bash
SHOME=/home/$USER/git/librteye2

#$SHOME/salmaprv/scripts/set_cpu_performance.sh
#LD_LIBRARY_PATH=$SHOME:$SHOME/salmaprv $SHOME/gui/librteye_gui 1>$SHOME/librteye2stdout 2>$SHOME/librteye2stderr
#LD_LIBRARY_PATH=$SHOME $SHOME/gui/librteye_gui #1>$SHOME/librteye2stdout 2>$SHOME/librteye2stderr
$SHOME/librteye2/gui/librteye_gui #1>$SHOME/librteye2stdout 2>$SHOME/librteye2stderr
#$SHOME/salmaprv/scripts/set_cpu_powersave.sh
