{
  description = "Dev shell for toolkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "toolkit";

        buildInputs = with pkgs; [
          clang
          clang-tools
          lld
          libcxx

          cmake
          fmt
          pkg-config
          hwloc
          perf
          gnumake
          python313
        ];
        shellHook = ''
          export SHELL=${pkgs.zsh}/bin/zsh
          export CC=clang
          export CXX=clang++
          export CLANGD_FLAGS="$CLANGD_FLAGS --query-driver=/nix/store/*-clang-wrapper-*/bin/clang++"
        '';
      };
    };
}
