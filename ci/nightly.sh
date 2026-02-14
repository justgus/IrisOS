#!/bin/sh
set -eu
cd /home/justgus/Dev/irisos-ai/referee
git pull --rebase
./bootstrap.sh
./configure
make -j
make check
make distcheck