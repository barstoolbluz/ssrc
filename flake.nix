{
  description = "SSRC - Shibatch Sample Rate Converter";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        sleefSrc = pkgs.fetchFromGitHub {
          owner = "shibatch";
          repo = "sleef";
          rev = "0c063a8f0e01c22fa1e473effd2e7a0c69b4963a";
          sha256 = "sha256-BM88tQAImtQuKKBSPT0hE659cfe6FVzxKSF3TSrbknk=";
          fetchSubmodules = true;
        };

        sleef = pkgs.stdenv.mkDerivation {
          pname = "sleef";
          version = "4.0.0";

          src = sleefSrc;

          nativeBuildInputs = with pkgs; [ cmake ];
          buildInputs = with pkgs; [ openssl ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DSLEEF_BUILD_DFT=ON"
            "-DSLEEF_ENFORCE_DFT=ON"
            "-DSLEEFDFT_ENABLE_PARALLELFOR=ON"
            "-DSLEEF_BUILD_TESTS=OFF"
          ];

          postInstall = ''
            # Handle lib64 to lib migration if needed
            if [ -d "$out/lib64" ] && [ "$(ls -A $out/lib64)" ]; then
              cp -r $out/lib64/* $out/lib/ || true
              rm -rf $out/lib64
            fi
          '';
        };

        ssrc = pkgs.stdenv.mkDerivation rec {
          pname = "ssrc";
          version = "2.4.2";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            git
          ];

          buildInputs = with pkgs; [
            openssl
            sleef
          ];

          # Provide pre-built SLEEF so CMake doesn't try to download it
          preConfigure = ''
            mkdir -p submodules
            cp -r ${sleefSrc} submodules/sleef
            chmod -R u+w submodules/sleef
          '';

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DBUILD_TESTS=ON"
            "-DBUILD_CLI=ON"
          ];

          # Help CMake find the pre-built SLEEF
          CMAKE_PREFIX_PATH = "${sleef}";

          postInstall = ''
            # Handle lib64 to lib migration if needed
            if [ -d "$out/lib64" ] && [ "$(ls -A $out/lib64)" ]; then
              cp -r $out/lib64/* $out/lib/ || true
              rm -rf $out/lib64
            fi
          '';

          meta = with pkgs.lib; {
            description = "Shibatch Sample Rate Converter - audiophile-grade sample rate converter";
            homepage = "https://github.com/shibatch/SSRC";
            license = licenses.boost;
            platforms = platforms.unix;
            mainProgram = "ssrc";
          };
        };

      in {
        packages = {
          default = ssrc;
          ssrc = ssrc;
        };

        apps = {
          default = {
            type = "app";
            program = "${ssrc}/bin/ssrc";
          };
          ssrc = {
            type = "app";
            program = "${ssrc}/bin/ssrc";
          };
          scsa = {
            type = "app";
            program = "${ssrc}/bin/scsa";
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ ssrc ];
          packages = with pkgs; [
            cmake
            ninja
            git
            gcc13
          ];

          shellHook = ''
            echo "SSRC development environment"
            echo "Version: ${ssrc.version}"
            echo ""
            echo "Available commands:"
            echo "  cmake -B build -G Ninja"
            echo "  cmake --build build"
            echo "  nix build      # Build the package"
            echo "  nix run        # Run ssrc"
          '';
        };
      }
    );
}
