#!/bin/bash
# The convention for our Conan packages is to name libraries `@rpath/lib*.dylib`.
# This script renames them to point to the specific Vuo framework subdirectory for the current Vuo version.

set -o errexit
set -o pipefail

PROJECT_SOURCE_DIR="$1"
shift
MODULES_DEST_DIR="$1"
shift

for i in "$@"; do
	dylib=$(basename "$i")

	chmod +w "$i"

	install_name_tool -id "@rpath/$MODULES_DEST_DIR/$dylib" "$i" \
		2>&1 | (grep -F -v 'will invalidate the code signature' || true)

	rpathsToFix=$(otool -L "$i" | tail +3 | (grep -E --only-matching '@rpath/[^ ]+' || true))
	for j in $rpathsToFix; do
		destDylib=$(basename "$j")
		install_name_tool -change "$j" "@rpath/$MODULES_DEST_DIR/$destDylib" "$i" \
			2>&1 | (grep -F -v 'will invalidate the code signature' || true)
	done

	# Add an ad-hoc code-signature, replacing the existing signature if any
	# (if there was one, it was invalidated by the above install_name_tool-ing).
	"$PROJECT_SOURCE_DIR/base/codesignWrapper.sh" --sign - --force "$i" \
		2>&1 | (grep -F -v ': replacing existing signature' || true)

	chmod -w "$i"
done
