{ gcc
, clang-tools
, mkShell
}: mkShell {
  buildInputs = [ gcc clang-tools ];
}
