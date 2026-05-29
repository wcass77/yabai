# Scroll Layout

This fork adds a `scroll` layout for horizontally scrollable workspaces. Scroll layout is a managed layout, like `bsp` and `stack`, but it represents each managed window as a horizontal column in one long strip.

## Enabling Scroll Layout

Set the active space to scroll layout:

```sh
yabai -m space --layout scroll
```

Set the default layout for spaces that do not have an explicit layout override:

```sh
yabai -m config layout scroll
```

Scroll layout is only valid for user spaces. It cannot be applied to macOS native fullscreen spaces.

## Mental Model

A scroll space has:

- a list of columns, one managed window per column
- a focused column index
- a viewport offset
- a width for each column

The viewport is centered on the focused column when possible. At the left and right boundaries, the viewport is clamped so it does not scroll past the strip.

Windows outside the viewport are still managed. When a column is fully outside the viewport, yabai leaves an 8-pixel edge peek rather than moving it completely out of view.

## Column Geometry

Each column fills the scroll area's height. The scroll area is the display's constrained frame after applying space padding.

New columns get a default width based on:

```text
floor(scroll_area_width * split_ratio)
```

with a minimum width of `1`.

Column gaps use the normal space/window gap setting. Padding and gap toggles affect scroll spaces the same way they affect other managed spaces.

Examples:

```sh
yabai -m config split_ratio 0.50
yabai -m space --gap abs:12
yabai -m space --padding abs:10:10:10:10
```

## Navigation

Use `space --scroll` to move the viewport by column:

```sh
yabai -m space --scroll prev
yabai -m space --scroll next
yabai -m space --scroll first
yabai -m space --scroll last
```

`west` and `east` are aliases for `prev` and `next`:

```sh
yabai -m space --scroll west
yabai -m space --scroll east
```

Scrolling to a visible scroll column also focuses that column's window. Boundary scrolls fail with an error rather than wrapping.

Window focus commands that resolve previous, next, first, last, west, or east use the scroll column order while on a scroll space.

## Focus Behavior

When a managed window in a scroll space receives focus, yabai updates the focused column and recenters the viewport on that column. This applies whether focus comes from a yabai command, clicking a window, focus-follows-mouse, or application focus changes.

If the space is visible, yabai flushes the new geometry immediately. If the space is not visible, it marks the view dirty and applies the layout when needed.

## Insertion Behavior

When a window becomes managed in a scroll space, yabai creates a new column for it.

Insertion follows the normal `window_insertion_point` setting:

| Setting | Behavior in scroll layout |
| --- | --- |
| `first` | Insert at the beginning of the column list. |
| `last` | Insert at the end of the column list. |
| `focused` | Insert after the focused column when a focused column exists. |

If an explicit insertion point is supplied internally, the new column is inserted after that column.

Focus preservation is intentional:

- if the inserted window is the focused window, the new column becomes focused
- if the inserted window is not focused and it is inserted before the focused column, the focused index shifts so the same window remains focused
- the new window becomes the view's insertion point

The BSP-only `window --insert` directional insertion command is not supported on scroll spaces.

## Removing Windows

When a managed scroll window is removed, its column is removed. Focus moves predictably:

- removing the focused column focuses the next column at the same index when possible
- removing the last focused column focuses the new last column
- removing a column before the focused column shifts the focused index left
- removing the final column leaves no focused column

## Swap and Warp

`window --swap` works within a single scroll space by swapping column order. If either swapped window is focused, focus transfers to the other swapped window so the focused visual position remains natural.

`window --warp` works within a single scroll space by moving the acting column relative to the selected column:

- warping a column rightward places it after the target column
- warping a column leftward places it before the target column
- if the moved column was focused, focus follows it
- if another column was focused, yabai adjusts the focused index so the same window remains focused

Scroll swap/warp does not move columns between spaces. Both windows must be in the same scroll view.

## Resizing

Scroll spaces support horizontal column resizing only:

```sh
yabai -m window --resize left:-40:0
yabai -m window --resize right:40:0
```

Left-handle resizing subtracts the x delta from the column width. Right-handle resizing adds the x delta. Width is clamped to a minimum of `1`.

Unsupported resize modes on scroll spaces:

- top or bottom handle resizing
- absolute resizing
- ratio adjustment

## Unsupported BSP Operations

These commands are not supported on scroll spaces:

- `space --balance`
- `space --equalize`
- `space --mirror`
- `space --rotate`
- `window --stack`
- `window --insert`
- `window --ratio`
- `window --toggle split`

`window --grid` and `window --move` remain floating-window operations; they are rejected for managed scroll windows in the same way as other managed windows.

## Query Behavior

Scroll spaces report their type as `scroll`:

```sh
yabai -m query --spaces --space
```

The normal `windows`, `first-window`, and `last-window` fields use scroll column order.

## Runtime Relayout

If a scroll-managed window moves or resizes outside yabai's own animation, yabai compares the observed frame against the expected scroll geometry. If the window is still animating toward the expected position, relayout is deferred. Otherwise, the scroll view is flushed again.

This keeps scroll layout from fighting its own animations while still correcting windows that drift out of place.

## Example Keybindings

With `skhd`, a basic scroll setup might look like:

```sh
alt - h : yabai -m space --scroll prev
alt - l : yabai -m space --scroll next
shift + alt - h : yabai -m window --warp west
shift + alt - l : yabai -m window --warp east
ctrl + alt - h : yabai -m window --resize left:-80:0
ctrl + alt - l : yabai -m window --resize right:80:0
```

Use the bindings that match your existing focus and movement conventions.
