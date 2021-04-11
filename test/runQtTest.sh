#!/bin/bash
set -o pipefail

options=(-nocrashhandler -maxwarnings 10000)
if [ "$(basename "$1")" = "TestVuoVideo" ]; then
	options+=(-minimumvalue 1000)
fi

# Transform QBENCHMARK results like
#      0.000093 msecs per iteration (total: 98, iterations: 1048576)
#      0.03 events
# into data that CTest parses and includes in `Testing/**/*.xml`.
"$@" "${options[@]}" \
	 | perl -pe 's/(^ +([.0-9]+) ([^(]+ per iteration|events$))/<DartMeasurement name="\3" type="numeric\/double">\2<\/DartMeasurement>\1/g'
