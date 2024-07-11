{ markdown
, python312
, stdenv
}: stdenv.mkDerivation {
  name = "www";
  src = ./pages;
  buildInputs = [ markdown python312 ];
  buildPhase = ''
    find "$src" -type f -name "*.md" | while read -r file; do
      rel_path="''${file#$src/}"
      dest_file="$out/''${rel_path%.md}.html"
      mkdir -p "$(dirname "$dest_file")"
      markdown "$file" > "$dest_file"
    done
  '';
}
