#!/bin/bash
set -o nounset
set -o errexit

binaryDir="$1"
outputDir="$2"
exampleName="$3"
if [ -z "$binaryDir" ] || [ -z "$outputDir" ] || [ -z "$exampleName" ]; then
	echo "Instead of invoking this script directly, use:"
	echo "    ctest -R TestExample_QuickStart"
	exit 1
fi

# Compile and link the composition.
"$binaryDir/vuo-compile-for-framework" "$exampleName.vuo" -o "$outputDir/$exampleName.bc"
"$binaryDir/vuo-link" --optimization fast-build-existing-cache "$outputDir/$exampleName.bc"

# Start the composition.
"$outputDir/$exampleName" &

# Wait for the composition's window to appear.
windowID=""
attempts=0
while [ -z "$windowID" ] && [ $attempts -lt 10 ] ; do
	sleep 1s
	windowID="$(getwindowid "$exampleName" "$exampleName" || true)"
	attempts=$((attempts+1))
done

# Take a screenshot of the window.
if [ "$windowID" != "" ]; then
	sleep 1s
	screenshotFile="$binaryDir/$exampleName.png"
	screencapture -l$windowID "$screenshotFile" 2>&1 \
		| (grep -v "^libpng warning" || true)
fi

# Stop the composition.
pkill "$exampleName" || (echo "error: $exampleName wasn't running after $attempts seconds" ; false)
