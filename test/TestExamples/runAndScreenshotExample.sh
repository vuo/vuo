#!/bin/bash

example="$1"
binaryDir="$(dirname $1)"
exampleName="$(basename $1)"
waitSeconds=10

$example &
sleep ${waitSeconds}s
windowID="$(getwindowid "$exampleName" "$exampleName")"
if [ "$windowID" != "" ]; then
	screenshotFile="$binaryDir/$exampleName.png"
	screencapture -l$windowID "$screenshotFile" 2>&1 \
		| grep -v "^libpng warning"
fi
pkill $exampleName || (echo "error: $exampleName wasn't running after $waitSeconds seconds" ; false)
