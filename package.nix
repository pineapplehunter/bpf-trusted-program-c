{
  lib,
  stdenv,
  libbpf,
  pkg-config,
  llvmPackages,
  makeWrapper,
}:

stdenv.mkDerivation {
  name = "trust-filter";
  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.unions [
      ./Makefile
      ./trust-filter-common.h
      ./trust-filter.bpf.c
      ./trust-filter.c
      ./vmlinux.h
    ];
  };

  nativeBuildInputs = [
    pkg-config
    llvmPackages.clang-unwrapped
    makeWrapper
  ];
  buildInputs = [ libbpf ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin $out/lib
    cp trust-filter $out/bin/
    cp trust-filter.bpf.o $out/lib/
    wrapProgram "$out/bin/trust-filter" \
      --add-flag "$out/lib/trust-filter.bpf.o"
    runHook postInstall
  '';

  meta = {
    description = "eBPF LSM program that logs executed binaries and their security.bpf.trust xattr";
    license = lib.licenses.gpl2Only;
    platforms = lib.platforms.linux;
    maintainers = [ ];
  };
}
