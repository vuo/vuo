#!/bin/bash
# Runs `codesign`, retrying a few times if it fails
# (since it occasionally segfaults or incorrectly reports "no identity found").

set -o nounset

for _ in {1..5} ; do
	/usr/bin/codesign "$@"
	status=$?
	if [ $status -eq 0 ]; then
		break
	fi
	sleep 5
	echo "codesign failed; retrying"
done

if [ $status -ne 0 ]; then
	echo "error: codesign failed after 5 tries"
	exit 1
fi
