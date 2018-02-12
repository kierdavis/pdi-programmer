with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "env";
  buildInputs = [
    (python27.withPackages (pyPkgs: with pyPkgs; [ pyserial ]))
  ];
}

