{
  description = "Dev shell for toolkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
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
          cmake
          clang-tools
          clang
          fmt
          hwloc
        ];
        shellHook = ''
          export SHELL=${pkgs.zsh}/bin/zsh
        '';
      };
    };
}
