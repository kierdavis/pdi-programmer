{ pkgs ? import <nixpkgs> {}, device ? "atmega644p" }:

with pkgs;

stdenv.mkDerivation rec {
  name = "env";

  buildInputs = [ avrbinutils avrdude avrgcc avrlibc ];

  CC_FLAGS = [ "-isystem ${avrlibc}/avr/include" ];

  shellHook = ''
    lib=$(find ${avrlibc}/avr/lib -name "lib${device}.a" -print)
    if [ -z "$lib" ]; then
      echo >&2 "Could not find target-specific library in ${avrlibc}/avr/lib (bad `device`?)."
      exit 1
    fi
    libdir=$(dirname $lib)
    export LD_FLAGS="-B $libdir -L $libdir"
  '';
}
