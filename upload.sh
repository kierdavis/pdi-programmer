#!/bin/sh

set -o errexit -o pipefail -o nounset

platform=$1
source ./common.sh

avrdude $AVRDUDE_FLAGS -U flash:w:$build_dir/pdiprog.hex
