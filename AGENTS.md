# Agent Notes

This fork has local workflows that are not obvious from upstream yabai documentation. Follow these notes when working in this repository.

## Repository Remotes

- `origin` is `wcass77/yabai`.
- `upstream` is `asmvik/yabai`.
- Fetch both remotes before PR or integration work:

  ```sh
  git fetch --all --prune
  ```

For current-branch PR discovery, use `gh pr status` and `gh pr list --head <branch>`.

## Integration Workflow

For PRs or branch work that may interact with upstream changes:

1. Create a temporary integration branch from `origin/master`.
2. Merge `upstream/master` first.
3. Resolve conflicts and run validation.
4. Merge the PR branch.
5. Run validation again.
6. Fast-forward or merge `master` only after the combined state is sane.

Expected local validation for code changes:

```sh
make -C tests
make
```

The current test suite includes 14 tests after the BSP hidden sibling regression and scroll layout tests.

## Commit Signing

Local git commit signing may block on 1Password UI. For automation-created merge commits, use:

```sh
git commit --no-gpg-sign
```

Do not leave an interactive signing process running.

## yabai-dev

This fork uses `yabai-dev` as its active local yabai launcher. See [doc/yabai-dev.md](doc/yabai-dev.md).

Important behavior:

- `yabai-dev activate` builds, signs, updates sudoers, loads the scripting addition, and restarts launchd.
- `yabai-dev activate --no-build` uses the existing `./bin/yabai` without rebuilding or re-signing it.
- Re-signing changes the binary sha256 and therefore invalidates the sudoers hash.
- If sudoers installation fails, run the exact `sudo install ...` command printed by `yabai-dev`, then rerun `yabai-dev activate --no-build`.
- Do not rerun plain `yabai-dev activate` after installing a printed sudoers file unless you intend to rebuild/re-sign and install a new sudoers hash.

Useful checks:

```sh
yabai-dev status
yabai -m query --displays
readlink ~/.config/yabai/dev/bin/yabai
```

If yabai starts but `query --displays` cannot connect, check for a stale socket or half-started launchd state:

```sh
launchctl bootout "gui/$(id -u)" ~/Library/LaunchAgents/com.asmvik.yabai.plist 2>/dev/null || true
rm -f "/tmp/yabai_${USER}.socket" "/tmp/yabai_${USER}.lock"
launchctl bootstrap "gui/$(id -u)" ~/Library/LaunchAgents/com.asmvik.yabai.plist
```

## Scroll Layout

This fork has a `scroll` layout. See [doc/scroll-layout.md](doc/scroll-layout.md).

When changing scroll behavior, update the scroll docs and run:

```sh
make -C tests
```
