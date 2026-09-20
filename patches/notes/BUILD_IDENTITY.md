# Build identity

## Architecture and supplied inputs

- Architecture: x86_64
- SSRC base: `b769add0756157ea88d1bbf9023e06c437485a79`
- SLEEF source revision: `0c063a8f0e01c22fa1e473effd2e7a0c69b4963a`
- SSRC source archive SHA-256: `0dc29dc9aa0fe1c751e1f2e3feaa3bd333f3b608f61c5f211b38dfa4bd2b4897`
- Dependency bundle SHA-256: `5ce6474f0e18ed9747c14e8290d5598ae80bfae07e2302caff06d5717fc35816`

## Source acquisition note

The build container had no outbound GitHub/DNS access, so a literal in-container `git clone` was not possible. The canonical GitHub repository was checked separately and its current `master` was confirmed as `b769add0756157ea88d1bbf9023e06c437485a79`. The user then supplied `ssrc-b769add-with-git.tar.gz`, including `.git`; its SHA-256 matched the separately supplied checksum, its worktree was clean at that exact revision, and its SLEEF gitlink was the expected `0c063a8f...` revision. All patch/commit operations were performed from that exact Git history.

## Toolchain

- `gcc`: `/nix/store/qdwim37sf31raprhmrjgrqfhppl7d5vb-gcc-wrapper-15.2.0/bin/gcc`
- GCC: 15.2.0
- `g++`: `/nix/store/qdwim37sf31raprhmrjgrqfhppl7d5vb-gcc-wrapper-15.2.0/bin/g++`
- G++: 15.2.0
- linker: `/nix/store/qdwim37sf31raprhmrjgrqfhppl7d5vb-gcc-wrapper-15.2.0/bin/ld`
- GNU ld / binutils: 2.44
- CMake: 4.1.2
- Ninja: 1.13.1
- pkg-config: 0.29.2

## Production build

CMake configuration:

```text
-G Ninja
-DCMAKE_BUILD_TYPE=Release
-DBUILD_TESTS=ON
-DBUILD_CLI=ON
```

Effective SSRC Release C++ flags include the repository's `-Wall -fcompare-debug-second` plus CMake's `-O3 -DNDEBUG`.

The executable reports:

```text
Version = 2.4.2
Build info = GNU 15.2.0 2026-09-20T02:24:02Z Release
```

Final clean re-applied executable:

```text
SHA-256 c13b9e38f8c759fa07d93efcac34f476890de868f4f9a13453385460c13a1f2e
```

The base executable used for baseline/performance comparison was built with the same supplied GCC 15.2.0 environment. Its SHA-256 was:

```text
325bdb4c662f19b002b1ebe3217879a9de48d2842af7c3a1efee50d58eeccecf
```

## SLEEF use in the fresh verification checkout

The exact pinned SLEEF source was built from the supplied bundle. For the final fresh `git am` verification checkout, the already-built static SLEEF 4.0.0 artifacts under `/usr/local/lib64` were reused to avoid repeatedly rebuilding the same dependency within the execution wrapper.

SLEEF's installed `sleef.pc` in this environment advertises only `-lsleef`, while SSRC also links `sleefdft` and `tlfloat`. A temporary build-environment-only pkg-config file therefore exposed the already-built exact libraries as:

```text
-L/usr/local/lib64 -lsleefdft -lsleef -ltlfloat
```

This shim was outside the repository, is not part of the patch series, and does not change source identity.
