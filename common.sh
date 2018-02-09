function show() {
  echo "$@"
  "$@"
}

top_dir=$(dirname $0)
build_dir=$top_dir/build
mkdir -p $build_dir

platform_dir=$top_dir/platform/$platform

src_dirs="$top_dir/src $platform_dir"

# Include path
for src_dir in $src_dirs; do
  CC_FLAGS="${CC_FLAGS:-} -I$src_dir"
done

# Optimisations
CC_FLAGS="${CC_FLAGS:-} -Os -flto"
LD_FLAGS="${LD_FLAGS:-} -Os -flto"

# Warnings
CC_FLAGS="${CC_FLAGS:-} -Wall -Werror"
LD_FLAGS="${LD_FLAGS:-} -Wall -Werror"

# Language standard
C_FLAGS="${C_FLAGS:-} -std=c11"
CPP_FLAGS="${CPP_FLAGS:-} -std=c++11"

source $platform_dir/vars.sh

# Device
CC_FLAGS="${CC_FLAGS:-} -mmcu=$MCU"
LD_FLAGS="${LD_FLAGS:-} -mmcu=$MCU"
AVRDUDE_FLAGS="${AVRDUDE_FLAGS:-} -p $MCU"
