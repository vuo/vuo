#!/bin/bash

open package/Vuo.app

windowID=""
attempts=0
while [ -z "$windowID" -a $attempts -lt 10 ] ; do
	sleep 2s
	windowID="$(getwindowid "Vuo" "Untitled Composition")"
	attempts=$[$attempts+1]
done
sleep 1s
screencapture -l$windowID test/TestVuoEditor/VuoEditor.png

killall Vuo
