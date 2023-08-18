#!/bin/bash

# Specify the file you want to iterate through
#filename="tmp_all_src_files.txt"
#filename2="CMakeLists.txt"

filename="tmp_cmake_files.txt"
filename2="PolyVR_splitted_22.04.cbp"

# Check if the file exists
if [ ! -f "$filename" ]; then
    echo "File not found: $filename"
    exit 1
fi

# Read and process each line in the file
while IFS= read -r line; do
	l=$(echo -n $line | tr -d '\r')
	e=$(echo -n $l | tail -c 1)
	if [ "$e" = "h" ]; then
		continue
	fi
	
	if grep -q "$l" "$filename2"; then
		:
	else
		echo "$l   not found in $filename2"
	fi
	
done < "$filename"
