{ python27Packages }:

python27Packages.buildPythonApplication rec {
  name = "pdiprog-${version}";
  version = "0.1";

  src = ./src;

  propagatedBuildInputs = with python27Packages; [ pyserial ];
}
