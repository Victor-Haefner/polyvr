#!/bin/bash

# sudo apt install exif
rm imgTable.dat

function parseCoord {
	IFS='|' read -r -a arr <<< "$1"
	IFS=',' read -r -a arr <<< "${arr[1]}"
	res=""
    	for e in "${arr[@]}"
	do
		part=$( echo $e | sed 's/ //g' )
		res="$res|$part"
	done
	echo $res
}

function processImage {
	filename="$1"

	params=$( exif "$filename" )
	latRaw=$( echo "$params" | grep Latitude )
	lonRaw=$( echo "$params" | grep Longitude )
	
	lat=$( parseCoord "$latRaw" )
	lon=$( parseCoord "$lonRaw" )
	echo "$filename|$lat|$lon" >> imgTable.dat
}

find . -name "*.jpg"|while read fname; do
	processImage "$fname"
done


