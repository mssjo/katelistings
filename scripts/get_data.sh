#!/bin/bash

mkdir -p themes
mkdir -p syntaxes

# Download kate data
wget https://invent.kde.org/frameworks/syntax-highlighting/-/archive/master/syntax-highlighting-master.tar\
        -O kate_data.tar
        
# Extract colour themes
tar -xf kate_data.tar -C themes syntax-highlighting-master/data/themes --strip-components=3
# Extract syntaxes
tar -xf kate_data.tar -C syntaxes/ syntax-highlighting-master/data/syntax --strip-components=3

rm -v kate_data.tar

# Sort syntaxes into sections
sections="3D Assembler Configuration Database Hardware Markup Other Scientific Scripts Sources"
for section in $sections
do
    echo "$section:"
    mkdir -p syntaxes/$section
    
    grep -F -l "section=\"$section\"" syntaxes/*.xml | \
    while read -r filename ; do
        echo "    $filename"
        mv $filename syntaxes/$section
    done
done

for theme in themes/*
do
    echo $theme
done

#Map languages and extensions
map_languages.sh
