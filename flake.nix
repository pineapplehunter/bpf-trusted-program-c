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
        { pkgs, lib, ... }:
        {
          devShells.default = pkgs.mkShell {
            packages = with pkgs; [
              bpftools
              glibc
              libbpf
              libelf
              llvmPackages.bintools
              llvmPackages.clang-unwrapped
              pkg-config
              clang-tools
            ];
            env.CPATH =
              with pkgs;
              "${lib.makeIncludePath [
                clang
                glibc
                libbpf
                linuxHeaders
              ]}";
          };

          packages = {
            trust-filter = pkgs.callPackage ./package.nix { };
            default = pkgs.callPackage ./package.nix { };
          };

          formatter = pkgs.nixfmt-tree;
        };
    };
}
