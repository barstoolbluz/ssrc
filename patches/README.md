# Downstream patches (vendored)

This directory carries the Tonepoet **finite-stream / floating-`fact`** repair as a
downstream patch, applied at *build time* so the source tree stays pristine and the
change is easy to refactor and forward-port onto future SSRC releases.

## What it fixes

Against base revision `b769add0756157ea88d1bbf9023e06c437485a79`:

1. **Floating-point `fact` metadata** — RIFF, Wave64, and RF64 floating output now
   carry a standards-correct `fact` chunk whose frame count equals the data actually
   written (backpatched at close, after the async writer drains).
2. **Finite time-aligned output** — linear-phase conversion publishes the finite
   destination timeline `round_half_up(N * Fd / Fs)` instead of serializing filter
   pre/post-roll. For the 12,544-frame High 44.1→48 kHz case: **35,499 → 13,653 frames.**

Verified: `git am` reproduces topic tree `2323ef992784ec072709c136221ac4954cafb4bd`;
**114/114** tests pass; all three containers emit correct `fact`/`ds64`. See `notes/`.

## Files

| path | purpose |
|---|---|
| `ssrc-tonepoet-finite-stream.patch` | combined unified diff — **this is what the build applies** (`git apply` / `patch -p1`) |
| `series/000{1,2,3}-*.patch`, `series/series` | `git format-patch` mailbox series for `git am` and for rebasing onto new releases |
| `series/0000-cover-letter.patch` | records the base commit |
| `notes/` | timing-contract derivation, test/perf/qualification evidence, Tonepoet repin checklist |
| `scripts/inspect_wave.py` | independent byte-level RIFF/RF64/Wave64 structure inspector |

## How it is wired into the build

Both Nix build paths apply the combined patch during `patchPhase`:

- `flake.nix` → `patches = [ ./patches/ssrc-tonepoet-finite-stream.patch ];`
- `.flox/pkgs/ssrc.nix` → `patches = [ ../../patches/ssrc-tonepoet-finite-stream.patch ];`

So `nix build` and `flox build ssrc` produce the fixed binaries while `src/` stays at
the upstream base. (A plain `nix develop` + manual `cmake` builds the *unpatched* tree;
apply the patch yourself first if you want the fix in an ad-hoc dev build:
`git apply patches/ssrc-tonepoet-finite-stream.patch`.)

## Forward-porting to a new SSRC release

Prefer the mailbox series so authorship/messages survive:

```sh
git switch -c tonepoet/finite-stream-contract <new-base>
git am -3 patches/series/0001-*.patch patches/series/0002-*.patch patches/series/0003-*.patch
# resolve any conflict, then: git am --continue
```

Then regenerate the combined patch the build consumes:

```sh
git diff <new-base>..HEAD > patches/ssrc-tonepoet-finite-stream.patch
git format-patch -o patches/series <new-base>..HEAD
```

Likely conflict hotspots (see `notes/TIMING_CONTRACT.md` for the derivation):
`src/libshibatchdsp/xdr_wav.h` (writer init/finalize, RIFF/W64/RF64 sizes, `fact`
placement, RF64 `ds64`), `SRC.hpp` (LCM-clock delay decomposition, finite clamp),
`FastPP.hpp` (polyphase phase origin), `src/tester/CMakeLists.txt` (spectral windows +
new test registration). Do **not** resolve conflicts by reintroducing threshold-based
silence trimming, profile-specific output lengths, or a post-hoc file rewrite.

## Alternative: bake it into the fork instead of vendoring

If you later decide the fork's source itself should carry the fix (rather than applying
at build time), `git am` the series onto the source and drop the `patches = [...]` lines
from `flake.nix` and `.flox/pkgs/ssrc.nix`.
