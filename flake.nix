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

      version = "0.1.0";

      mkBpfls =
        pkgs:
        pkgs.callPackage ./nix/bpfls.nix {
          src = self;
          inherit version;
        };
      mkVscodeExtension = pkgs: pkgs.callPackage ./nix/vscode-extension.nix { src = ./vscode; };
    in
    {
      packages = forAllSystems (pkgs: {
        bpfls = mkBpfls pkgs;
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

      # `nix flake check` runs every test: the unit tests (in the bpfls
      # build) and the end-to-end tests, which start the real binary.
      checks = forAllSystems (
        pkgs:
        let
          inherit (self.packages.${pkgs.stdenv.hostPlatform.system}) bpfls vscode-extension;
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
