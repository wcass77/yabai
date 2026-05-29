# yabai-dev

`yabai-dev` is the development launcher used by this fork to build, sign, activate, and roll back local yabai worktrees. It keeps the user-facing `yabai` command stable while allowing the active binary to point at any checkout or worktree.

This document describes the behavior of the `yabai-dev` helper currently installed at `~/.local/bin/yabai-dev`.

## Paths

`yabai-dev` uses these paths:

| Path | Purpose |
| --- | --- |
| `~/.local/bin/yabai-dev` | User command for this helper. |
| `~/.local/bin/yabai` | Symlink to the active dev yabai binary. |
| `~/.config/yabai/dev/bin/yabai` | Stable launch target. This is a symlink to the active worktree's `bin/yabai`. |
| `~/.config/yabai/dev/state/` | Active/previous worktree state. |
| `~/.config/yabai/dev/log/` | Temporary files and generated sudoers snippets. |
| `~/Library/LaunchAgents/com.asmvik.yabai.plist` | Launch agent for the dev service. |
| `/private/etc/sudoers.d/yabai` | Passwordless `--load-sa` rule for the signed active binary hash. |

The launch agent runs `~/.config/yabai/dev/bin/yabai`, not a Homebrew binary.

## Commands

### `yabai-dev bootstrap`

Initializes the dev environment:

- verifies the required SIP/NVRAM state for scripting-addition usage
- stops/removes older Homebrew-era yabai service wiring where present
- writes the launch agent
- writes a default `~/.config/yabai/yabairc`, backing up any existing file first
- creates `~/.local/bin/yabai` and `~/.local/bin/yabai-dev`
- verifies that the configured codesign identity is usable

The default signing identity is `yabai-cert`, or the value of `YABAI_CERT` if set.

### `yabai-dev activate [options]`

Builds, signs, points the stable symlink at the current checkout, updates sudoers, loads the scripting addition, restarts launchd, and verifies that yabai responds to `query --displays`.

When `--no-build` is supplied, activation uses the existing `./bin/yabai` exactly as it is. It does not rebuild or re-sign the binary.

Run it from a yabai checkout root:

```sh
cd /path/to/yabai
yabai-dev activate
```

Options:

| Option | Behavior |
| --- | --- |
| `--no-build` | Use the existing `./bin/yabai` instead of rebuilding or re-signing. |
| `--debug` | Build with `make all` instead of `make install`. |
| `--identity NAME` | Codesign with `NAME` instead of `yabai-cert`. |
| `--no-restart` | Activate files and state but skip launchd restart. |

### `yabai-dev rebuild`

Re-enters the recorded active worktree and runs `activate` there.

```sh
yabai-dev rebuild
```

Any activation options can be passed through, for example:

```sh
yabai-dev rebuild --debug
```

### `yabai-dev rollback`

Activates the recorded previous worktree. If that worktree already has `bin/yabai`, rollback uses `--no-build`; otherwise it rebuilds.

```sh
yabai-dev rollback
```

### `yabai-dev status [--verbose]`

Prints the current dev state and health checks:

- active and previous worktrees
- stable symlink target
- active version and sha256
- codesign verification
- sudoers / scripting-addition load status
- launchd service status
- extra yabai processes
- `query --displays` result
- SIP/NVRAM status
- Homebrew yabai presence
- `command -v yabai`

Use verbose mode for launchd details, codesign metadata, sudoers contents when readable, logs, and display query output:

```sh
yabai-dev status --verbose
```

### `yabai-dev stop`

Stops the dev launch agent:

```sh
yabai-dev stop
```

## Activation Details

Activation performs these steps in order:

1. Check SIP/NVRAM requirements.
2. Build `bin/yabai` unless `--no-build` is supplied.
3. Clear quarantine attributes and codesign the binary when a build was performed.
4. Verify the binary's code signature.
5. Point `~/.config/yabai/dev/bin/yabai` at the checkout's `bin/yabai`.
6. Generate a sudoers line for the active binary hash.
7. Install `/private/etc/sudoers.d/yabai`.
8. Run `sudo -n ~/.config/yabai/dev/bin/yabai --load-sa`.
9. Update dev state files.
10. Restart launchd unless `--no-restart` is supplied.
11. Verify `query --displays`.

The sudoers rule is hash-bound. Re-signing the binary changes the hash, even when the source code did not change. Use `--no-build` when you need to preserve the existing binary hash.

## Sudoers Recovery

If activation fails while installing sudoers, `yabai-dev` prints a command like:

```sh
sudo install -o root -g wheel -m 0440 ~/.config/yabai/dev/log/sudoers.yabai.XXXXXX /private/etc/sudoers.d/yabai
```

Run the exact command it prints from an interactive terminal. After installing it, rerun activation with `--no-build` so the existing binary hash is preserved:

```sh
yabai-dev activate --no-build
```

When recovering manually, prefer this sequence:

1. Run the printed `sudo install ...` command.
2. Verify the active binary hash still matches the generated sudoers file:

   ```sh
   shasum -a 256 ~/.config/yabai/dev/bin/yabai
   cat ~/.config/yabai/dev/log/sudoers.yabai.XXXXXX
   ```

3. Either rerun activation without rebuilding:

   ```sh
   yabai-dev activate --no-build
   ```

   or continue manually by loading the scripting addition:

   ```sh
   sudo -n ~/.config/yabai/dev/bin/yabai --load-sa
   ```

4. Restart launchd:

   ```sh
   launchctl bootout "gui/$(id -u)" ~/Library/LaunchAgents/com.asmvik.yabai.plist 2>/dev/null || true
   rm -f "/tmp/yabai_${USER}.socket" "/tmp/yabai_${USER}.lock"
   launchctl bootstrap "gui/$(id -u)" ~/Library/LaunchAgents/com.asmvik.yabai.plist
   ```

5. Verify:

   ```sh
   yabai-dev status
   yabai -m query --displays
   ```

If activation was finished manually instead of through `yabai-dev activate --no-build`, make sure the state files in `~/.config/yabai/dev/state/` match the active worktree, binary, and hash.

## Common Checks

Confirm the active binary:

```sh
readlink ~/.config/yabai/dev/bin/yabai
yabai --version
```

Confirm launchd is running:

```sh
launchctl print "gui/$(id -u)/com.asmvik.yabai"
```

Confirm the command socket works:

```sh
yabai -m query --displays
```

Confirm there are no extra dev yabai processes:

```sh
ps -axo pid,command | grep '[y]abai'
```

## Notes

- `yabai-dev activate --no-build` skips compilation and signing; it preserves the existing binary hash.
- `yabai-dev status` intentionally calls `sudo -n ... --load-sa`; this verifies sudoers and the scripting addition without prompting.
- The helper is designed for local development worktrees. Use `rollback` if a newly activated build does not behave correctly.
- If yabai starts but `query --displays` cannot connect, check for a stale socket or half-started service, then restart launchd cleanly.
