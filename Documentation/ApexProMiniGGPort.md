# Wired Apex Pro Mini GG protocol integration

Source: https://github.com/newnetmp3/ApexProMini_gg_rgb, particularly `src/lib.rs`, `docs/PROTOCOL.md`, and `docs/PRESET_COVERAGE.md`.

## Implementation on this branch

- USB `1038:161e`, HID interface 1, usage page `0xFFC0`, usage 1.
- The dedicated wired Mini direct-lighting path sends a zero-padded 513-byte HID Feature report: `00 40 NN` followed by up to 68 `[HID usage, R, G, B]` entries.
- The supported 68 HID usage codes are copied from the independently implemented research serializer, not from proprietary source.
- Colors are looked up by HID usage in the existing OpenRGB 111-entry LED array, preventing incorrect positional mapping.
- Direct lighting release uses a 65-byte HID Output report beginning `00 41`.
- Mini initialization, brightness reads and region queries are bypassed pending model-specific evidence. The ANSI Mini SKU `64820` is used for the existing OpenRGB region patch.
- Other Apex models continue to use their previous transport paths.

## Hardware acceptance

**Not hardware-tested in OpenRGB yet.** On an Arch Linux workstation with the wired Mini attached:

1. Stop `apex-mini-ripple.service` and other applications that write keyboard RGB, to avoid competing HID writes.
2. Build the branch using OpenRGB's documented build instructions and check compilation before launching it.
3. Confirm only one Apex Pro Mini device appears and the OpenRGB keyboard matrix shows the intended ANSI Mini positions.
4. In Direct mode, set A red, S green, and then the entire keyboard blue. Confirm all visually; check modifier, Fn and Space keys individually.
5. Switch to Onboard mode and quit OpenRGB. Verify the previous onboard lighting returns.
6. Inspect logs for HID write errors, and reconnect the keyboard if lighting remains in direct mode.

No firmware, actuation, macros, or device profile writes are required.

## Follow-up

Add automated packet serialization tests and verify matrix SKU/region accuracy. The 31 historical GG Prism identifiers and 14 independent effect algorithms in the research project require a software effects layer; they are not native onboard modes and are not exact vendor shader reproductions. Reactive effects require an explicit, privacy-conscious key event source. Do not copy proprietary SteelSeries GG descriptors or shaders.
