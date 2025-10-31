{ stdenv
, cmake
, git
, openssl
, fetchFromGitHub
, lib
}:

let
  sleefSrc = fetchFromGitHub {
    owner = "shibatch";
    repo = "sleef";
    rev = "0c063a8f0e01c22fa1e473effd2e7a0c69b4963a";
    sha256 = "sha256-BM88tQAImtQuKKBSPT0hE659cfe6FVzxKSF3TSrbknk=";
    fetchSubmodules = true;
  };

  sleef = stdenv.mkDerivation {
    pname = "sleef";
    version = "4.0.0";

    src = sleefSrc;

    nativeBuildInputs = [ cmake ];
    buildInputs = [ openssl ];

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

in stdenv.mkDerivation rec {
  pname = "ssrc";
  version = "2.4.2";

  src = ../../.;

  nativeBuildInputs = [
    cmake
    git
  ];

  buildInputs = [
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

  meta = with lib; {
    description = "Shibatch Sample Rate Converter - audiophile-grade sample rate converter";
    homepage = "https://github.com/shibatch/SSRC";
    license = licenses.boost;
    platforms = platforms.unix;
    mainProgram = "ssrc";
  };
}
