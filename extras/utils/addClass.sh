#!/bin/sh

# example:
#  ./addClass.sh addons/Engineering/Machining/MachiningCode

function remAndMake {
	if [ -e $1 ]; then
		rm $1
	fi
	touch $1
}

function appendTo {
	echo "$2" >> "$1"
}

className="VR$(basename "$1")"
srcPath="../../src"
classDir="$srcPath/$(dirname "$1")"
classPath="$classDir/$className"

echo "class name: $className"
echo "class dir: $classDir"

CLASSNAME=${className^^}

mkdir -p "$classDir"

remAndMake "$classPath.h"
remAndMake "$classPath.cpp"

#ls "$classDir"

appendTo "$classPath.h" "#ifndef ${CLASSNAME}_H_INCLUDED"
appendTo "$classPath.h" "#define ${CLASSNAME}_H_INCLUDED"
appendTo "$classPath.h" ""
appendTo "$classPath.h" "#include <memory>"
appendTo "$classPath.h" ""
appendTo "$classPath.h" "using namespace std"
appendTo "$classPath.h" ""
appendTo "$classPath.h" "class $className : public std::enable_shared_from_this<$className> {"
appendTo "$classPath.h" "	private:"
appendTo "$classPath.h" "	public:"
appendTo "$classPath.h" "		$className();"
appendTo "$classPath.h" "		~$className();"
appendTo "$classPath.h" ""
appendTo "$classPath.h" "		${className}Ptr create();"
appendTo "$classPath.h" "		${className}Ptr ptr();"
appendTo "$classPath.h" "}"
appendTo "$classPath.h" ""
appendTo "$classPath.h" "#endif ${CLASSNAME}_H_INCLUDED"

appendTo "$classPath.cpp" "#include \"$className.h\""
appendTo "$classPath.cpp" ""
appendTo "$classPath.cpp" "$className::$className() {}"
appendTo "$classPath.cpp" "$className::~$className() {}"
appendTo "$classPath.cpp" ""
appendTo "$classPath.cpp" "${className}Ptr create() { return ${className}Ptr( new $className() ); }"
appendTo "$classPath.cpp" "${className}Ptr ptr() { return shared_from_this(); }"

#less "$classPath.h"





