{
  description = "bpfls - a language server for bpftrace";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  # bpftrace, lsp-framework and cppspec are git submodules the build needs.
  inputs.self.submodules = true;

  outputs =
    { self, nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (s: f nixpkgs.legacyPackages.${s});

      version = "0.2.1";

      # Written as callPackage functions so the result has .override
      # (e.g. bpfls-static below).
      mkBpfls =
        pkgs:
        pkgs.callPackage (
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
            # Links a fully static binary that runs on any x86_64/aarch64
            # Linux, for release artifacts. LLVM is already linked
            # statically; this adds glibc, libstdc++ and zlib.
            static ? false,
            # Builds and runs the unit tests. Off for installs, which then
            # skip compiling cppspec and the test executables; on for
            # `nix flake check` (see checks below).
            withTests ? false,
          }:
          let
            # cppspec (tests/cppspec) downloads these with CMake's
            # FetchContent, which the build sandbox has no network for, so
            # they are fetched here and handed to CMake instead. prettyprint
            # is pinned to the commit cppspec's `master` resolved to when
            # this was written.
            cxx-prettyprint = fetchFromGitHub {
              owner = "louisdx";
              repo = "cxx-prettyprint";
              rev = "9ab26d228f2960f50b38ad37fe0159b7381f7533";
              hash = "sha256-4R5q0KlcvXPMvs6MZUZ8ByHpuoNRHCcGiRUsh7gv4q4=";
            };
          in
          stdenv.mkDerivation {
            pname = "bpfls";
            inherit version;
            src = self;

            nativeBuildInputs = [
              cmake
              ninja
              # Applies bpfls's patches to bpftrace during the build.
              git
            ];

            cmakeFlags = [
              "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"
            ]
            ++ lib.optionals withTests [
              "-DBPFLS_BUILD_TESTS=ON"
              # cppspec asks for argparse v3.2, the version nixpkgs packages.
              "-DFETCHCONTENT_SOURCE_DIR_ARGPARSE=${argparse.src}"
              "-DFETCHCONTENT_SOURCE_DIR_CXX-PRETTYPRINT=${cxx-prettyprint}"
            ]
            ++ lib.optionals static [
              "-DCMAKE_EXE_LINKER_FLAGS=-static"
              # LLVM's CMake package finds zlib itself; point it at the
              # archive.
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

            # With withTests, the unit tests run in the build; the end-to-end
            # tests are a separate check (see checks.e2e below).
            doCheck = withTests;
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
        ) { };

      mkVscodeExtension =
        pkgs:
        pkgs.callPackage (
          {
            lib,
            buildNpmPackage,
            importNpmLock,
            vscode-utils,
          }:
          let
            src = ./vscode;
            manifest = lib.importJSON "${src}/package.json";

            # The extension's files plus its npm dependencies
            # (vscode-languageclient), fetched one by one with the integrity
            # hashes in vscode/package-lock.json. Unlike npmDepsHash this
            # needs no updating when the lock file changes, which it does on
            # every release (see pre_bump_hooks in cog.toml).
            files = buildNpmPackage {
              pname = "bpfls-vscode-files";
              inherit (manifest) version;
              inherit src;
              npmDeps = importNpmLock { npmRoot = src; };
              inherit (importNpmLock) npmConfigHook;
              dontNpmBuild = true;
              installPhase = ''
                runHook preInstall
                mkdir -p $out
                cp -r package.json extension.js language-configuration.json \
                  node_modules $out/
                runHook postInstall
              '';
            };
          in
          vscode-utils.buildVscodeExtension {
            pname = "bpfls";
            inherit (manifest) version;
            src = files;
            vscodeExtPublisher = manifest.publisher;
            vscodeExtName = manifest.name;
            vscodeExtUniqueId = "${manifest.publisher}.${manifest.name}";

            # The source is a directory, not a .vsix; put it where the
            # builder looks.
            unpackPhase = ''
              runHook preUnpack
              cp -r ${files} extension
              chmod -R u+w extension
              runHook postUnpack
            '';

            meta = {
              description = "bpftrace language support in VS Code, using bpfls";
              homepage = "https://github.com/anakin4747/bpfls";
              license = lib.licenses.asl20;
              platforms = lib.platforms.linux;
            };
          }
        ) { };
    in
    {
      packages = forAllSystems (pkgs: {
        bpfls = mkBpfls pkgs;
        # The release binary (see .github/workflows/release.yml).
        bpfls-static = (mkBpfls pkgs).override { static = true; };
        vscode-extension = mkVscodeExtension pkgs;
        default = self.packages.${pkgs.stdenv.hostPlatform.system}.bpfls;
      });

      # Adds bpfls and, following nixpkgs' layout, vscode-extensions.bpfls.bpfls
      # for use with e.g. vscode-with-extensions.
      overlays.default = final: prev: {
        bpfls = mkBpfls final;
        vscode-extensions = prev.lib.recursiveUpdate prev.vscode-extensions {
          bpfls.bpfls = mkVscodeExtension final;
        };
      };

      # `nix flake check` runs every test: the unit tests (in a bpfls build
      # with tests) and the end-to-end tests, which start the real binary.
      checks = forAllSystems (
        pkgs:
        let
          inherit (self.packages.${pkgs.stdenv.hostPlatform.system}) vscode-extension;
          bpfls = self.packages.${pkgs.stdenv.hostPlatform.system}.bpfls.override { withTests = true; };
        in
        {
          inherit bpfls vscode-extension;
          e2e = bpfls.overrideAttrs {
            pname = "bpfls-e2e";
            checkPhase = ''
              runHook preCheck
              ctest --output-on-failure --label-regex e2e --no-tests=error
              runHook postCheck
            '';
            installPhase = ''
              runHook preInstall
              touch $out
              runHook postInstall
            '';
          };
        }
      );

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            cereal
            cloc
            cmake
            cocogitto
            curl
            elfutils.dev
            gcc
            git
            gnumake
            jq
            libbpf
            llvm.dev
            neovim
            ninja
            nodejs
            vscodium
          ];
        };
      });
    };
}
