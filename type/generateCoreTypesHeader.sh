#!/bin/bash

echo '// This header is generated by vuo/type/generateCoreTypesHeader.sh.'

for header in $* ; do
	TYPE=${header%.h}
	echo "#include \"$header\""
done
