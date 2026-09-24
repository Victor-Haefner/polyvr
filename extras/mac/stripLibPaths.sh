#!/bin/bash

strip_absolute_paths() {
    local executable=$1
    local libs=$(otool -L "$executable" | grep -oE '/[^ ]+\.dylib' | sort -u)

    # Loop through each library and strip the absolute path
    for lib in $libs; do
        local lib_name=$(basename "$lib")
        #install_name_tool -change "$lib" "$lib_name" "$executable"
    done
}

check_libs_paths() {
		echo "check_libs_paths $1"
		local alllibs=()

		for file in $1/*.dylib; do
        if [[ -f $file ]]; then
						local file_name=$(basename "$file")
            #echo "Processing file: $file_name"
				    local libs=$(otool -L "$file" | awk '{print $1}' | grep '\.dylib' | sort -u)

				    for lib in $libs; do

								if [[ "$lib" == *":" ]]; then # this is itself
										continue
								fi

								if [[ "$lib" == "/usr/lib/"* ]]; then
										continue
								fi

								if [[ "$lib" == *"libSystem"* ]]; then
										continue
								fi

								if [[ "$lib" == *"libc++"* ]]; then
										continue
								fi

								if [[ "$lib" == *"libgfortran"* ]]; then
										continue
								fi


				        local lib_name=$(basename "$lib")
				        #echo " dependency: $lib_name"
								if [[ ! -f $1/$lib_name ]]; then
										#echo "missing file: $lib_name"
										alllibs+=("$lib")
				        fi
				    done

						#break
        fi
    done

		unique_libs=($(printf "%s\n" "${alllibs[@]}" | sort -u))

		echo "Missing libraries:"
		for lib in "${unique_libs[@]}"; do
				echo "$lib"
				local lib_name=$(basename "$lib")
				#find -L /opt/homebrew -name "$lib_name"
				#grep "$lib_name" $1/*.dylib

				#if [[ "$lib" == *"homebrew"* ]]; then
				#		cp "$lib" $1/
				#fi
		done
}

#strip_absolute_paths $1
check_libs_paths $1
