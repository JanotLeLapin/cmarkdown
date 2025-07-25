{ gcc
, stdenv
}: stdenv.mkDerivation {
  pname = "cmarkdown";
  version = "0.1";

  buildInputs = [ gcc ];
  src = ./.;

  buildPhase = ''
    gcc -c cmarkdown.c -o cmarkdown.o
    gcc -shared -o libcmarkdown.so cmarkdown.o
  '';
  installPhase = ''
    mkdir -p $out/lib
    mkdir -p $out/include
    cp libcmarkdown.so $out/lib
    cp cmarkdown.h $out/include
  '';
}
