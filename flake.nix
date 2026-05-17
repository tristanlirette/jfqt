{
  description = "jfqt - Qt6 C++ GUI wrapper for jftui";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "jfqt";
          version = "0-unstable-${self.shortRev or "dirty"}";
          src = self;

          nativeBuildInputs = with pkgs; [
            cmake
            qt6.wrapQtAppsHook
          ];

          buildInputs = with pkgs; [
            qt6.qtbase
            jftui
          ];

          qtWrapperArgs = [
            "--prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.jftui ]}"
          ];

          meta = {
            description = "Qt6 GUI wrapper for jftui Jellyfin client";
            homepage = "https://github.com/tristanlirette/jfqt";
            platforms = pkgs.lib.platforms.linux;
            mainProgram = "jfqt";
          };
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            qt6.full
            gcc
            pkg-config
          ];
        };
      }
    );
}
