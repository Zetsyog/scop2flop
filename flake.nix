{
  description = "A flake for using Pesto";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs =
    { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_18;

        ntl = llvm.stdenv.mkDerivation rec {
          pname = "ntl";
          version = "11.5.1";
          src = pkgs.fetchurl {
            url = "https://www.shoup.net/ntl/ntl-${version}.tar.gz";
            sha256 = "sha256-IQ0GwxMGy8bq9oFEU8Vsd22djo3zbXTrMG9qUj0caoo=";
          };
          buildInputs = [
            pkgs.gmp
          ];
          nativeBuildInputs = [
            pkgs.perl # needed for ./configure
          ];
          sourceRoot = "${pname}-${version}/src";

          enableParallelBuilding = true;

          dontAddPrefix = true; # DEF_PREFIX instead
          configurePlatforms = [ ];
          configurePhase = ''
            ./configure PREFIX=$out NATIVE=off NTL_GMP_LIP=on CXX=${llvm.stdenv.cc.targetPrefix}c++
          '';

          doCheck = false;
        };

        barvinok = llvm.stdenv.mkDerivation rec {
          pname = "barvinok";
          version = "0.41.8";
          src = pkgs.fetchurl {
            url = "https://barvinok.sourceforge.io/barvinok-${version}.tar.gz";
            sha256 = "sha256-zED+7lqILqn7ruaWswPzd+nEayMhgxMDSAvtGtuOyjo=";
          };
          buildInputs = [
            llvm.libcxxClang
            llvm.libcxx
            llvm.libcxxStdenv
            ntl
            pkgs.gmp
            pkgs.libtool
            pkgs.automake
            pkgs.autoconf
            pkgs.bison
            pkgs.flex
            pkgs.glpk
          ];
          enableParallelBuilding = true;
          nativeBuildInputs = [ pkgs.pkgconf ];
          configurePhase = ''
            ./configure --prefix=$out --disable-shared-barvinok CC=${llvm.clang}/bin/clang CXX=${llvm.clang}/bin/clang++ LD=${llvm.clang}/bin/ld
          '';
          buildPhase = ''
            make -j
          '';
          installPhase = ''
            make install
          '';
        };

      in
      {
        devShells.default = pkgs.mkShell.override { stdenv = llvm.stdenv; } {

          packages = with pkgs; [
            git

            llvm.clang-tools
            llvm.stdenv
            llvm.clang
            llvm.libcxx
            llvm.bintools
            llvm.clang-manpages
            llvm.openmp

            libyaml
            gmp
            cmake
            autoconf
            automake
            pkgconf
            libtool
            bison
            flex

            glpk

            # trahrhe
            ntl

            cmake-format

            barvinok

          ];
          shellHook = ''

          '';
        };
      }
    );
}
