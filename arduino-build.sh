#!/bin/bash
PROJECT=Seeed_Arduino_edgeimpulse.ino
BOARD=Seeeduino:samd:seeed_wio_terminal
COMMAND=$1

INCLUDE="-I ./src"
INCLUDE+=" -I./src/model-parameters"
INCLUDE+=" -I ./src/repl"
INCLUDE+=" -I ./src/ingestion-sdk-c/"
INCLUDE+=" -I ./src/ingestion-sdk-c/inc"
INCLUDE+=" -I ./src/ingestion-sdk-c/inc/signing"
INCLUDE+=" -I ./src/ingestion-sdk-platform/wio-terminal"
INCLUDE+=" -I ./src/sensors"
INCLUDE+=" -I ./src/QCBOR/inc"
INCLUDE+=" -I ./src/QCBOR/src"
INCLUDE+=" -I ./src/cm_backtrace"
INCLUDE+=" -I ./src/mbedtls_hmac_sha256_sw/"

INCLUDE+=" -I ./src/cm_backtrace"


FLAGS+=" -O3"
FLAGS+=" -g3"
FLAGS+=" -DEI_SENSOR_AQ_STREAM=int"
FLAGS+=" -DEIDSP_QUANTIZE_FILTERBANK=0"
FLAGS+=" -D__STATIC_FORCEINLINE=__STATIC_INLINE"


if [ "$COMMAND" = "--build" ];
then
	echo "Building $PROJECT"
	arduino-cli compile --fqbn  $BOARD --build-property build.project_flags="$INCLUDE $FLAGS" $PROJECT &
	pid=$! # Process Id of the previous running command
	while kill -0 $pid 2>/dev/null
	do
		echo "Still building..."
		sleep 2
	done
	wait $pid
	ret=$?
	if [ $ret -eq 0 ]; then
		echo "Building $PROJECT done"
	else
		exit "Building $PROJECT failed"
	fi
elif [ "$COMMAND" = "--flash" ];
then
	arduino-cli upload -p $(arduino-cli board list | grep Arduino | cut -d ' ' -f1) --fqbn $BOARD -i *.bin
elif [ "$COMMAND" = "--all" ];
then
	arduino-cli compile --fqbn  $BOARD $PROJECT
	status=$?
	[ $status -eq 0 ] && arduino-cli upload -p $(arduino-cli board list | grep Arduino | cut -d ' ' -f1) --fqbn $BOARD -i *.bin
else
	echo "Nothing to do for target"
fi
