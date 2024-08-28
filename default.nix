{ gcc
, stdenv
}: stdenv.mkDerivation {
  pname = "cmarkdown";
  version = "0.1";

  buildInputs = [ gcc ];
  src = ./.;

  buildPhase = ''
    gcc cmarkdown.c -o cmarkdown
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp cmarkdown $out/bin
  '';
}
