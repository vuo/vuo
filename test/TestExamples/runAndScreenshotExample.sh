#!/usr/bin/env -S PATH=/opt/homebrew/bin:/usr/local/bin:/bin:/usr/bin:/usr/sbin bash
set -o nounset
set -o errexit

binaryDir="$1"
outputDir="$2"
exampleName="$3"
useGMalloc="$4"
if [ -z "$binaryDir" ] || [ -z "$outputDir" ] || [ -z "$exampleName" ] || [ -z "$useGMalloc" ]; then
	echo "Instead of invoking this script directly, use:"
	echo "    ctest -R TestExample_QuickStart"
	exit 1
fi

# Compile and link the composition.
"$binaryDir/vuo-compile-for-framework" "$exampleName.vuo" -o "$outputDir/$exampleName.bc"
"$binaryDir/vuo-link" --optimization existing-module-caches "$outputDir/$exampleName.bc"

# Start the composition.
if [ "$useGMalloc" == "ON" ]; then
	export DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib
fi
"$outputDir/$exampleName" &
export -n DYLD_INSERT_LIBRARIES

# Wait for the composition's window to appear.
windowID=""
attempts=0
while [ -z "$windowID" ] && [ $attempts -lt 10 ] ; do
	sleep 1
	windowID="$(getwindowid "$exampleName" "$exampleName" || true)"
	attempts=$((attempts+1))
done

# Take a screenshot of the window.
if [ "$windowID" != "" ]; then
	sleep 1
	screenshotFile="$outputDir/$exampleName.png"
	screencapture -l$windowID "$screenshotFile" 2>&1 \
		| (grep -v "^libpng warning" || true)
fi

# Stop the composition.
pkill "$exampleName" || (echo "error: $exampleName wasn't running after $attempts seconds" ; false)
