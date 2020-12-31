#!/bin/bash
# The convention for our Conan packages is to name libraries `@rpath/lib*.dylib`.
# This script renames them to point to the specific Vuo framework subdirectory for the current Vuo version.

set -o errexit

MODULES_DEST_DIR=$1
shift

for i in "$@"; do
	dylib=$(basename "$i")

	chmod +w "$i"

	install_name_tool -id "@rpath/$MODULES_DEST_DIR/$dylib" "$i" \
		2>&1 | (grep -F -v 'will invalidate the code signature' || true)

	rpathsToFix=$(otool -L "$i" | tail +3 | egrep --only-matching '@rpath/[^ ]+' || true)
	if [ $? -eq 0 ]; then
		for j in $rpathsToFix; do
			destDylib=$(basename "$j")
			install_name_tool -change "$j" "@rpath/$MODULES_DEST_DIR/$destDylib" "$i" \
				2>&1 | (grep -F -v 'will invalidate the code signature' || true)
		done
	fi

	chmod -w "$i"
done
