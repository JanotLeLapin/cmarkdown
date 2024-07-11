{ gcc
, clang-tools
, valgrind
, mkShell
}: mkShell {
  buildInputs = [ gcc clang-tools valgrind ];
}
