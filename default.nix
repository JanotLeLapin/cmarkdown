{ markdown-config
, gcc
, stdenv
}: stdenv.mkDerivation {
  pname = "markdown";
  version = "0.1";

  buildInputs = [ gcc ];
  src = ./.;

  buildPhase = ''
    cp ${markdown-config} config.h
    gcc markdown.c parse.c html.c util.c -o markdown -std=c99 -Wall
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp markdown $out/bin
  '';
}
