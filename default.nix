{ gcc
, stdenv
}: stdenv.mkDerivation {
  pname = "markdown";
  version = "0.1";

  buildInputs = [ gcc ];
  src = ./.;

  buildPhase = ''
    gcc markdown.c -o markdown
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp markdown $out/bin
  '';
}
