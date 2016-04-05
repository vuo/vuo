#!/bin/bash
# Find distributable binaries that reference local dylibs, and rewrite them.

QT_ROOT=$1
FRAMEWORKS_DEST_DIR=$2
QT_MAJOR_VERSION=$3

function fix()
{
	QT_FRAMEWORK="$1"
	TARGET_BINARY="$2"
	install_name_tool -change "$QT_ROOT/lib/$QT_FRAMEWORK.framework/Versions/$QT_MAJOR_VERSION/$QT_FRAMEWORK" "@rpath/$QT_FRAMEWORK.framework/Versions/$QT_MAJOR_VERSION/$QT_FRAMEWORK" "$TARGET_BINARY"
}

find $FRAMEWORKS_DEST_DIR -type f -perm +111 -print0 | while read -d $'\0' i ; do
	#echo "Checking $i"
	otool -L "$i" | grep '/usr/local/' > /dev/null
	if [ $? -eq 0 ]; then
		#echo "Fixing   $i"
		fix QtCore         "$i"
		fix QtGui          "$i"
		fix QtWidgets      "$i"
		fix QtMacExtras    "$i"
		fix QtPrintSupport "$i"
		fix QtOpenGL       "$i"
		fix QtXml          "$i"
		fix QtNetwork      "$i"
	fi
done
