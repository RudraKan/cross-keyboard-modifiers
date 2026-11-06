# cross-keyboard-modifiers

I like using two keyboards but the shift and command keys don't transfer between them. Karabiner-Elements and similar existing software wasn't allowed on my company laptop so I made this :)

Hold Shift / Cmd / Option / Control on one keyboard, press a key on the other, and it just works. It's ~100 lines of C, only touches keyboard events (never the mouse), and adds a few nanoseconds per keypress, so it won't affect gaming.

## Install

```sh
make install
```

Then turn on `cross-keyboard-modifiers` in **System Settings → Privacy & Security → Accessibility**. It starts automatically on login.

(If you rebuild it later, toggle it off and on again in that list.)

## Uninstall

```sh
make uninstall
```
