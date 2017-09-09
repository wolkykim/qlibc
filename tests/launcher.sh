#!/bin/sh

if [ $# = 0 ]; then
	echo "This program is called by make. Please use \"make test\" command instead."
	exit 1
fi

FAIL=0
FAILDESC=""
for EXECUTABLE in $*; do
	./$EXECUTABLE
	if [ $? != 0 ]; then
		FAIL=1
		FAILDESC="$FAILDESC $EXECUTABLE"
	fi
	echo ""
done

if [ $FAIL != 0 ]; then
	echo "======================================================================"
	echo "**** OOOOOPS!!! UNSUCESSFUL UNIT TEST FOUND. PLEASE FIX AND RERUN ****"
	echo "======================================================================"
	echo "Fails in =>$FAILDESC"
	exit 1
fi

echo "======================================================================"
echo "****             Good job! All tests are successful               ****"
echo "======================================================================"
echo "*   ____                 _     All tests have finished successfully. *"  
echo "*  / ___| ___   ___   __| |     | | ___ | |__   | |                  *"
echo "* | |  _ / _ \ / _ \ / _\` |  _  | |/ _ \| '_ \  | |                  *"
echo "* | |_| | (_) | (_) | (_| | | |_| | (_) | |_) | |_|                  *"
echo "*  \____|\___/ \___/ \__,_|  \___/ \___/|_.__/  (_)                  *"
echo "======================================================================"
echo "Tested: $*"
exit 0
