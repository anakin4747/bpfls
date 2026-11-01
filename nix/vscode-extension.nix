{
  lib,
  buildNpmPackage,
  importNpmLock,
  vscode-utils,
  src,
}:

let
  manifest = lib.importJSON "${src}/package.json";

  # The extension's files plus its npm dependencies (vscode-languageclient),
  # fetched one by one with the integrity hashes in vscode/package-lock.json.
  # Unlike npmDepsHash this needs no updating when the lock file changes,
  # which it does on every release (see pre_bump_hooks in cog.toml).
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

  # The source is a directory, not a .vsix; put it where the builder looks.
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
