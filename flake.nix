{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    nix-github-actions.url = "github:nix-community/nix-github-actions";
    nix-github-actions.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, nix-github-actions, ... }: let
    eachSystem = fn: nixpkgs.lib.genAttrs [
      "x86_64-linux"
      "x86_64-darwin"
    ] (system: (fn {
      inherit system;
      pkgs = (import nixpkgs { inherit system; } );
    }));
  in {
    githubActions = nix-github-actions.lib.mkGithubMatrix { checks = self.packages; };
    devShells = eachSystem ({ pkgs, ... }: { default = pkgs.callPackage ./shell.nix {}; });
    packages = eachSystem ({ pkgs, ... }: { default = pkgs.callPackage ./default.nix {}; });
  };
}
