#!/bin/bash

binaryDir="$(pwd)"

pngquant --force --speed 11 \
	"$binaryDir"/test/TestVuoEditor/*.png \
	"$binaryDir"/test/TestExamples/*.png

screenshotGalleryHTML="$binaryDir/screenshots.html"
echo '<head><style>div{float:left; background:#eee; margin:.5em; padding:.5em;} p{margin:0;} a{color:#000; text-decoration:none;}</style><title>Vuo Screenshots</title></head><body>' > $screenshotGalleryHTML
for i in "$binaryDir"/test/TestVuoEditor/*-or8.png \
		 "$binaryDir"/test/TestExamples/*-or8.png ; do
	exampleSubdir="$(dirname "$i")"
	exampleSubdir="${exampleSubdir#$binaryDir/}"
	exampleName="$(basename "$i" -or8.png)"
	mv "$i" "$exampleSubdir/$exampleName.png"
	echo '<div><a href="'$exampleSubdir/$exampleName.png'"><img src="'$exampleSubdir/$exampleName.png'" width=256 /><p>'$exampleName'</p></a></div>' >> $screenshotGalleryHTML
done
