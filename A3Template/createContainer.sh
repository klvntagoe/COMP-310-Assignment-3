#/bin/bash

$NAME="$1"
echo $NAME
docker run --privileged -d --rm --name=$NAME smean_image_comp310;
