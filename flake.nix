{
  description = "Rex BOF development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    # Renders docs/demo; pinned so re-rendering only changes when bumped.
    beaconcast.url = "github:rubyattack3r/beaconcast/v0.1.0";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      perSystem = { pkgs, inputs', ... }: {
        devShells.default = pkgs.mkShellNoCC {
          packages = [
            pkgs.actionlint
            pkgs.cppcheck
            pkgs.gnumake
            pkgs.pkgsCross.mingw32.stdenv.cc
            pkgs.pkgsCross.mingwW64.stdenv.cc
            pkgs.python3
            pkgs.shellcheck
            pkgs.unzip
            pkgs.zip
          ];
        };

        # Renders docs/demo with `nix develop .#demo -c make demo`.
        devShells.demo = pkgs.mkShellNoCC {
          packages = [
            inputs'.beaconcast.packages.default
            pkgs.gnumake
          ];
        };
      };
    };
}
