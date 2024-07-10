{ gcc
, stdenv
}: stdenv.mkDerivation {
  pname = "markdown";
  version = "0.1";

  buildInputs = [ gcc ];
  src = ./.;

  buildPhase = ''
    if [ ! -e config.h ]
    then
      cp config.def.h config.h
    fi
    gcc markdown.c html.c -o markdown
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp markdown $out/bin
  '';
}
