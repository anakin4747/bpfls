{
  lib,
  stdenv,
  cmake,
  ninja,
  git,
  llvm,
  libbpf,
  elfutils,
  cereal,
  argparse,
  fetchFromGitHub,
  glibc,
  zlib,
  src,
  version,
  # Links a fully static binary that runs on any x86_64/aarch64 Linux, for
  # release artifacts. LLVM is already linked statically; this adds glibc,
  # libstdc++ and zlib.
  static ? false,
}:

let
  # cppspec (tests/cppspec) downloads these with CMake's FetchContent, which
  # the build sandbox has no network for, so they are fetched here and handed
  # to CMake instead. prettyprint is pinned to the commit cppspec's `master`
  # resolved to when this was written.
  cxx-prettyprint = fetchFromGitHub {
    owner = "louisdx";
    repo = "cxx-prettyprint";
    rev = "9ab26d228f2960f50b38ad37fe0159b7381f7533";
    hash = "sha256-4R5q0KlcvXPMvs6MZUZ8ByHpuoNRHCcGiRUsh7gv4q4=";
  };
in
stdenv.mkDerivation {
  pname = "bpfls";
  inherit src version;

  nativeBuildInputs = [
    cmake
    ninja
    # Applies bpfls's patches to bpftrace during the build.
    git
  ];

  cmakeFlags = [
    "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"
    # cppspec asks for argparse v3.2, the version nixpkgs packages.
    "-DFETCHCONTENT_SOURCE_DIR_ARGPARSE=${argparse.src}"
    "-DFETCHCONTENT_SOURCE_DIR_CXX-PRETTYPRINT=${cxx-prettyprint}"
  ]
  ++ lib.optionals static [
    "-DCMAKE_EXE_LINKER_FLAGS=-static"
    # LLVM's CMake package finds zlib itself; point it at the archive.
    "-DZLIB_LIBRARY=${zlib.static}/lib/libz.a"
  ];

  buildInputs = [
    llvm.dev
    libbpf
    elfutils.dev
    cereal
  ]
  ++ lib.optionals static [
    glibc.static
    zlib.static
  ];

  # The unit tests run in the build; the end-to-end tests are a separate
  # check (see checks.e2e in flake.nix).
  doCheck = true;
  checkPhase = ''
    runHook preCheck
    ctest --output-on-failure --label-exclude e2e
    runHook postCheck
  '';

  meta = {
    description = "Language server for bpftrace";
    homepage = "https://github.com/anakin4747/bpfls";
    license = lib.licenses.asl20;
    mainProgram = "bpfls";
    platforms = lib.platforms.linux;
  };
}
