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
    lib = {
      markdown = eachSystem ({ pkgs, ... }: { config, ... }: pkgs.callPackage ./default.nix { markdown-config = config; });
    };
    devShells = eachSystem ({ pkgs, ... }: { default = pkgs.callPackage ./shell.nix {}; });
    packages = eachSystem ({ system, pkgs, ... }: {
      default = pkgs.callPackage ./default.nix { markdown-config = ./config.def.h; };
    });
  };
}
