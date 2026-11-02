# SA People release rules

- Apply gameplay, localization and bug fixes to the shared source used by both normal ASI and CLEO builds.
- Build and validate both editions after every update. Keep CLEO callback/API checks in addition to normal tests.
- Deliver two separate downloads: the normal complete ZIP, and a CLEO ZIP containing `Copy to game folder`.
- Never include the ASI in the CLEO download, or the CLEO plugin in the normal download.
- Use one English README per download. Preserve player settings, legacy archives and per-save worlds.
- Publish to GitHub only when requested; experimental CLEO stays a separate download.
