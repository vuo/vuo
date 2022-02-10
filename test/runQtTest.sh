#!/usr/bin/env -S PATH=/opt/homebrew/bin:/usr/local/bin:/bin:/usr/bin bash
set -o pipefail

options=(-nocrashhandler -maxwarnings 10000)
if [ "$(basename "$1")" = "TestVuoVideo" ]; then
	options+=(-minimumvalue 100)
else
	options+=(-minimumvalue 1000)
fi

# Transform QBENCHMARK results like
#      0.000093 msecs per iteration (total: 98, iterations: 1048576)  # emitted by QBENCHMARK {}
#      2,526 msecs per iteration (total: 2,526, iterations: 1)        # emitted by QBENCHMARK {}
#      842 msecs                                                      # emitted by QTest::setBenchmarkResult(…, QTest::WalltimeMilliseconds);
#      0.03 events                                                    # emitted by QTest::setBenchmarkResult(…, QTest::Events);
# into data that CTest parses and includes in `Testing/**/*.xml`.
"$@" "${options[@]}" \
	 | perl -p \
		-e 's/(\d+),(\d+) (msecs|msecs per iteration|events$)/\1\2 \3/g;' \
		-e 's/(^ +([.0-9]+) (msecs|msecs per iteration|events$))/<DartMeasurement name="\3" type="numeric\/double">\2<\/DartMeasurement>\1/g;'
