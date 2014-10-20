#!/bin/bash

#v1=$(grep -R "#include \"" )
v1=$(grep -R "#include <" )


#v1=$(echo "$v1" | awk '{ print $2 }')
v1=$(echo "$v1" | awk '{ n = split($0,a,/\.h/); if (n == 3) print $2; }')
v1=$(echo "$v1" | awk '{ n = split($0,a,/\.h/); if (n < 2) print $0; else print a[1] }')
v1=$(echo "$v1" | awk '{ n = split($0,a,/\//); if (n < 2) print $0; else print a[n] }')
v1=$(echo "$v1" | awk '{ n = split($0,a,/"/); if (n < 2) print $0; else print a[n] }')
v1=$(echo "$v1" | awk '{ n = split($0,a,/</); if (n < 2) print $0; else print a[n] }')
v1=$(echo "$v1" | awk '{ n = split($0,a,/>/); if (n < 2) print $0; else print a[n] }')
#echo "$v1"
echo "$v1" | sort | uniq -c | sort -rn
