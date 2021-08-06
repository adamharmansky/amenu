#!/bin/bash

for j in 512x512 256x256 192x192 128x128 96x96 64x64 48x48 32x32 24x24 16x16 ; do
	for i in $HOME/.local/share/icons/hicolor/$j/apps/*.png ; do
		if [ ! -e icons/48/$( basename $i .png ).ff ] ; then
			echo "converting $j icon $( basename $i .png ) ..."
			convert $i -resize 48x icons/48/$( basename $i .png ).ff
			convert $i -resize 24x icons/24/$( basename $i .png ).ff
		fi
	done
done
