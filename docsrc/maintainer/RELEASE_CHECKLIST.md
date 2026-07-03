# Release Checklist

Audience: library maintainers and release work.

1. Run host tests locally or confirm GitHub Actions is green.
2. Run `python scripts/run_socket_integration.py --compiler g++` locally if you want to verify the mock PLC normal and abnormal TCP paths before tagging.
3. Check `CHANGELOG.md` and move important items under the release version.
4. Check `API_POLICY.md` if any public API changed.
5. Confirm `library.properties` version and URL are correct.
6. Confirm `library.json` version matches `library.properties`.
7. If you are publishing to the PlatformIO Registry, confirm the registry does not already contain the same package version. Skipped version numbers are allowed; duplicate version publishing is not.
8. Confirm `README.md` examples, install steps, and size table still match the code.
9. Confirm `scripts/size_baseline.json` still matches the intended baseline.
10. Run Arduino library lint in strict submission mode if you plan to submit or update Arduino Library Manager.
11. Run `.\run_ci.bat --with-platformio` only when you intentionally validate embedded examples, PlatformIO metadata, or registry packaging.
12. Confirm all intended source, test, example, and workflow files are tracked in Git before tagging. The release archive is built from `git archive HEAD`, so untracked files will not ship.
13. Check GitHub repository metadata manually: description, website, topics, and pinned release.
14. Run `python scripts/release_notes.py --changelog CHANGELOG.md --version <version> --output release-notes.md` if you want to preview the release body locally.
15. Tag the release. `.github/workflows/release.yml` will publish the GitHub release artifacts from that tag.
