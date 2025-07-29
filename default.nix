{ stdenv
}: stdenv.mkDerivation {
  pname = "cmarkdown";
  version = "0.1";

  buildInputs = [];
  src = ./.;

  buildPhase = ''
    $CC -fPIC -c cmarkdown.c -o cmarkdown.o
    $CC -shared -o libcmarkdown.so cmarkdown.o
    ar rcs libcmarkdown.a cmarkdown.o
  '';
  installPhase = ''
    mkdir -p $out/lib
    mkdir -p $out/include
    cp libcmarkdown.so $out/lib
    cp libcmarkdown.a $out/lib
    cp cmarkdown.h $out/include
  '';
}
