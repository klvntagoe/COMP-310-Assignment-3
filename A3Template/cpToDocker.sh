#!/bin/bash

FILE="$1"

NAME="$2"

docker cp ~/COMP-310-Assignment-3/A3Template/$FILE $NAME:/home;
