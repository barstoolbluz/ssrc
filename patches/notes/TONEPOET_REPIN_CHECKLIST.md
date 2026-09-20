# Tonepoet re-pin checklist

Do these steps only after this patch series has been applied, committed, and pushed to the canonical `barstoolbluz/ssrc` fork.

1. Record the new canonical SSRC commit and set Tonepoet's `SSRC_REV` to that pushed revision. Do not use any local topic or `git am` commit ID from this bundle as the production pin unless that exact commit becomes canonical.
2. Recompute `SSRC_NAR` using the exact fetch expression Tonepoet currently uses for SSRC. Do not reuse the old fixed-output hash. If the expression fetches a GitHub archive, prefetch that exact canonical revision with the same unpacking semantics used by the expression and record the resulting NAR hash.
3. Update `flake.lock` using the repository's normal lock-update workflow, restricted to the SSRC input where practical.
4. Update `static_audit_rev4.json` source identity to the new canonical SSRC revision and newly fetched source identity.
5. Re-run the static source audit against the exact canonical revision.
6. Re-run the build audit with the approved compiler/toolchain and record the complete build identity.
7. Hash the newly built SSRC executable and replace the old executable digest in qualification/promotion evidence.
8. Run the SSRC Binary64 characterization/qualification against the new executable. The current command from the task brief is:

```text
python3 tonepoet-pipeline/qualification/ssrc_binary64/qualify_ssrc_binary64.py \
  --ssrc <new-ssrc-exe> \
  --tonepoet-w64-helper target/release/ssrc_w64_qualification \
  --characterize \
  --output <new-json>
```

9. Confirm that final qualification requires, rather than bypasses:
   - floating Wave64 `fact`;
   - exact `fact`/data frame-count agreement;
   - the finite timeline origin defined by the repaired SSRC contract;
   - the finite output frame rule documented in `TIMING_CONTRACT.md`;
   - DC/overload windows at predetermined timeline positions, not positions found by searching the signal.
10. Re-run promotion evidence and replace the previous SSRC characterization/build identity. The old evidence does not survive a source or executable identity change.
11. Archive the canonical SSRC revision, fixed-output source hash, `flake.lock`, static/build audits, executable SHA-256, characterization output, and promotion result together.

No future canonical revision or Tonepoet fixed-output hash is fabricated in this delivery.
