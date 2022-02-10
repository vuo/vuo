#!/usr/bin/env -S PATH=/opt/homebrew/bin:/usr/local/bin:/bin:/usr/bin bash
set -o nounset
set -o errexit
shopt -s nullglob

binaryDir="$(pwd)"

if [ -z "$(echo "$binaryDir"/test/TestVuoEditor/*.png "$binaryDir"/test/TestExamples/*.png)" ]; then
	exit 0
fi

pngquant --force --speed 11 \
	"$binaryDir"/test/TestVuoEditor/*.png \
	"$binaryDir"/test/TestExamples/*.png

screenshotGalleryHTML="$binaryDir/screenshots.html"
echo '<head><style>div{float:left; background:#eee; margin:.5em; padding:.5em;} p{margin:0;} a{color:#000; text-decoration:none;}</style><title>Vuo Screenshots</title></head><body>' > "$screenshotGalleryHTML"
for i in "$binaryDir"/test/TestVuoEditor/*-or8.png \
		 "$binaryDir"/test/TestExamples/*-or8.png ; do
	exampleSubdir="$(dirname "$i")"
	exampleSubdir="${exampleSubdir#$binaryDir/}"
	exampleName="$(basename "$i" -or8.png)"
	mv "$i" "$exampleSubdir/$exampleName.png"
	scaledHeight=192
	scaledWidth="$(echo "$(pngcheck "$exampleSubdir/$exampleName.png" | cut -d'(' -f2 | cut -d, -f1 | sed -e 's/x/\//') * $scaledHeight" | bc -l)"
	echo '<div><a href="'"$exampleSubdir/$exampleName.png"'"><img src="'"$exampleSubdir/$exampleName.png"'" width="'"$scaledWidth"'" height="'"$scaledHeight"'" loading="lazy" /><p>'"$exampleName"'</p></a></div>' >> "$screenshotGalleryHTML"
done
