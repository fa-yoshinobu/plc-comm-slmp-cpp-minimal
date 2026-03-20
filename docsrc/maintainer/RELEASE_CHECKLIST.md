# Release Checklist

Audience: library maintainers and release follow-up work.

1. Run host tests locally or confirm GitHub Actions is green.
2. Run `python scripts/run_socket_integration.py --compiler g++` locally if you want to verify the mock PLC normal and abnormal TCP paths before tagging.
3. Check `CHANGELOG.md` and move important items under the release version.
4. Check `API_POLICY.md` if any public API changed.
5. Confirm `library.properties` version and URL are correct.
6. Confirm `library.json` still matches the intended PlatformIO package metadata if you publish there.
7. Confirm `README.md` examples, install steps, and size table still match the code.
8. Confirm `scripts/size_baseline.json` still matches the intended baseline.
9. Run `python scripts/check_markdown_links.py` and confirm documentation links still resolve.
10. Run Arduino library lint in strict submission mode if you plan to submit or update Arduino Library Manager.
11. Validate the PlatformIO package with `pio pkg pack .` if you plan to publish there.
12. Confirm all intended source, test, example, and workflow files are tracked in Git before tagging. The release archive is built from `git archive HEAD`, so untracked files will not ship.
13. Check GitHub repository metadata manually: description, website, topics, and pinned release.
14. Run `python scripts/release_notes.py --changelog CHANGELOG.md --version <version> --output release-notes.md` if you want to preview the release body locally.
15. Tag the release. `.github/workflows/release.yml` will publish the GitHub release artifacts from that tag.
