{
  description = "BPF Trusted Program — C implementation using libbpf";

  inputs.nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  inputs.flake-parts.url = "github:hercules-ci/flake-parts";

  outputs =
    { flake-parts, ... }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-linux"
        "x86_64-linux"
      ];

      perSystem =
        { pkgs, system, ... }:
        {
          _module.args.pkgs = import inputs.nixpkgs {
            inherit system;
            overlays = [
              (final: prev: {
                libbpf = prev.libbpf.overrideAttrs {
                  version = "1.7.0";
                  src = final.fetchFromGitHub {
                    owner = "libbpf";
                    repo = "libbpf";
                    tag = "v1.7.0";
                    hash = "sha256-F92msxkYp4yZA3qUoSwS5GKUhcEO6DrYNln7w6U+jt0=";
                  };
                  patches = [ ];
                };
              })
            ];
          };

          devShells.default = pkgs.mkShell {
            packages = with pkgs; [
              clang
              cmake
              libbpf
              ninja
              pkg-config
              bpftools
              llvmPackages.bintools
            ];
            CLANG = "${pkgs.llvmPackages.clang-unwrapped}/bin/clang";
            SHELL = "${pkgs.bash}/bin/bash";
          };

          formatter = pkgs.nixfmt-tree;
        };
    };
}
