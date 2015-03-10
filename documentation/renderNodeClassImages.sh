#!/bin/bash

VUORENDER="$1"
NODE="$2"

# Specialize generic nodes, so we get their default constant values
if [ "$NODE" == "vuo.math.count" ]; then
	NODE="vuo.math.count.VuoReal"
fi

$VUORENDER --output-format=pdf --output image-generated/$NODE.pdf $NODE
$VUORENDER --output-format=png --output image-generated/$NODE.png $NODE
