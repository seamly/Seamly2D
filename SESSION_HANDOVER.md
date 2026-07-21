Here's the status at the end of the previous session:

## Task 20 — COMPLETE ✅ (merged 2026-07-21)

PR [#15](https://github.com/seamly/Seamly2D/pull/15) merged into `run-seamlyLayout` (merge commit `1b18a76a7`). The new **SeamlyLayout CI** workflow is live and verified by its own green run.

**What shipped:**

* **New workflow** `.github/workflows/seamlylayout-ci.yml` — a standalone, path-filtered GitHub Actions workflow that builds seamlyLayout (Rust core + Qt 6.10 QML frontend) and runs its tests on `ubuntu-latest` (Rust toolchain + `install-qt-action` Qt 6.10.1, `cmake --preset debug`, `ctest` under `xvfb`, `cargo test --workspace`). Kept **separate from `ci.yml`** (Qt 6.8.3) so the two toolchains stay independent.
* **Docs** — "Continuous integration (CI)" section in `README-BUILDS.md`, "SeamlyLayout CI" subsection in `README_WORKFLOWS.md`.
* **Task tracking** — Task 20 moved from `TODO_MIGRATE.md` to `COMPLETED.md`.

**Two cross-platform build bugs the new CI caught and fixed** (latent because the app had only ever been built on Windows):

1. **WebEngine deps** — `install-qt-action` was given only `qtwebengine`; aqtinstall does **not** auto-resolve a module's Qt deps, so `find_package(Qt6 … WebEngineQuick)` failed at configure (`Qt6WebChannel` missing). Fix: `modules: qtwebengine qtwebchannel qtpositioning`.
2. **QML module ↔ executable name collision** — the `qt_add_qml_module` URI equals the executable target name (`SeamlyLayout`), so Qt creates a `SeamlyLayout/` directory next to the binary. Windows dodges it via the `.exe` suffix; on Linux/macOS the binary is plain `SeamlyLayout`, so the final link failed with *"cannot open output file SeamlyLayout: Is a directory."* Fix: gave the module a distinct `OUTPUT_DIRECTORY` (`qml_modules/SeamlyLayout`) in `qt_frontend/CMakeLists.txt`, keeping the leaf name so `import SeamlyLayout` still resolves; runtime QML still loads from compiled-in resources. Verified with a local Windows debug build.

**CI result:** all PR checks green — SeamlyLayout CI (build + ctest + cargo test), plus the full `ci.yml` matrix (Windows/macOS/Linux builds, unit tests, AppImage, CodeQL). One transient `apt-get` flake in the AppImage job cleared on retry.

**Repo state:** local `run-seamlyLayout` synced to origin (`1b18a76a7`); local + remote `task20-seamlylayout-ci` branches deleted.

**Heads-up for future cross-platform CI work:** when macOS CI is added for seamlyLayout, the QML-module-dir collision (#2 above) would bite there too — it's already fixed in `CMakeLists.txt`, but keep it in mind if the module/target naming ever changes. Same for any Qt-module dep additions: list transitive deps explicitly for `install-qt-action`.
