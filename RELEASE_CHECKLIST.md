# Release Checklist

1. Run host tests locally or confirm GitHub Actions is green.
2. Run `python scripts/run_socket_integration.py --compiler g++` locally if you want to verify the mock PLC normal and abnormal TCP paths before tagging.
3. Check `CHANGELOG.md` and move important items under the release version.
4. Check `API_POLICY.md` if any public API changed.
5. Confirm `library.properties` version and URL are correct.
6. Confirm `README.md` examples and size table still match the code.
7. Confirm `scripts/size_baseline.json` still matches the intended baseline.
8. Confirm all intended source, test, example, and workflow files are tracked in Git before tagging. The release archive is built from `git archive HEAD`, so untracked files will not ship.
9. Run `python scripts/release_notes.py --changelog CHANGELOG.md --version <version> --output release-notes.md` if you want to preview the release body locally.
10. Tag the release. `.github/workflows/release.yml` will publish the GitHub release artifacts from that tag.
