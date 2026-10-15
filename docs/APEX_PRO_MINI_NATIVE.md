# SteelSeries Apex Pro Mini — native OpenRGB support

This document records the wired **SteelSeries Apex Pro Mini** controller validation
for the `feature/apex-pro-mini-steelseries-controller` branch. This branch adds
native device support; it does **not** bundle the independently maintained
Apex Pro Mini / GG effects plugins.

## Supported hardware and protocol

- Owner-tested wired USB device VID:PID **1038:161e**, on Arch Linux with
  an OpenRGB Qt5 build.
- Device detection uses the Mini's USB PID and vendor-defined HID interface 1;
  the serial/SKU prefix cannot be relied on to distinguish the Mini from
  some SteelSeries TKL models.
- Direct RGB uses a **513-byte feature report**, byte 1 = `0x40`, byte 2 =
  the address count, followed by 68 HID usage + RGB quadruplets.
- Onboard restoration uses a **65-byte output report** with byte 1 = `0x41`;
  sending this report via `hid_write()` returns lighting control to the
  firmware. The native implementation was derived from the separately tested
  Apex Pro Mini plugin's release packet.
- The physical ANSI layout shows **61 keys** in a 5 × 15 sparse matrix, while
  68 lighting addresses remain available to the protocol. Seven regional or
  alternative addresses are not displayed as physical keys. The Backspace
  physical key uses LED index **67**, not index 13.
- Onboard mode suppresses subsequent Direct-mode color reports.

## Hardware test record (owner-reported, 2026-10-15)

| Check | Outcome |
| --- | --- |
| OpenRGB identifies wired keyboard and firmware | Passed |
| Full-board dim-white direct lighting | Passed |
| Index 0 lights Escape only | Passed |
| Number row, QWERTY, home, Shift and modifier rows | Passed |
| Index 67 lights Backspace only | Passed |
| GUI selecting A illuminates only A | Passed |
| Direct-to-Onboard immediately restores stored firmware effect | Passed |
| Onboard-to-Direct color control | Passed |
| Native application exits without a segmentation fault | Passed with separate virtual-controller plugin disabled |
| Unplug/replug, rescan, return to Direct, then Onboard | Passed |
| Python source-level regression checks | Passed on owner's checkout |

The separate Apex Pro Mini plugin caused an OpenRGB shutdown crash in a
virtual-device worker thread. Disabling that plugin removed the crash.
That issue is not part of the native device controller changes.

## Regression test

From the repository root:

```sh
python3 -m unittest discover -s tests -p test_apex_pro_mini_native.py
```

This test checks source-level HID-order, matrix and report invariants. It
does not exercise USB hardware or replace the native OpenRGB build and
runtime tests.

## Limitations and upstream review

- The current validation covers **one wired ANSI** unit; other revisions
  and regional layouts are not established as compatible.
- The standard OpenRGB matrix UI shows uniform key cells rather than
  physical keycap widths or authentic keyboard-row staggering.
- Reconnection has been validated **after a device rescan**, not as
  automatic plug-and-play rediscovery in every environment.
- Firmware/profile storage, keyboard actuation configuration, and GG-style
  effect emulation are out of scope.
- Before merging upstream, run the repository's normal Linux/Qt5 build,
  applicable lint/CI checks, and confirm that other SteelSeries Apex models
  are unaffected. Owner hardware passes are not a substitute for upstream CI.

## Contribution process (OpenRGB CONTRIBUTING.md)

The canonical contribution target is **OpenRGB on GitLab**:
https://gitlab.com/CalcProgrammer1/OpenRGB . Prepare a merge request from a
feature branch, not a protected fork `master` branch. Rebase onto the
latest upstream `master` rather than merging master into the topic branch.
Keep this merge request limited to native wired Apex Pro Mini support.

Follow OpenRGB's established C-oriented C++ conventions (indexed loops,
C arrays for fixed tables, 4-space indentation, Qt only in UI sources,
LogManager diagnostics, and existing comment formatting). Build OpenRGB
with its QMake/Qt5 workflow and review the diff against existing Apex models.
Fork CI may be unavailable due to GitLab runner restrictions; record local
compile and device tests accurately rather than claiming unobserved CI passes.

**AI assistance disclosure for the submitter:** OpenRGB's CONTRIBUTING.md
states that AI-generated submissions are generally not permitted. All code
in this branch that was produced with AI assistance must be fully understood,
independently inspected, and manually vetted by the human contributor before
submission. Styling changes and passing automated tests do not by themselves
make such a submission acceptable. Follow upstream's policies on authorship
and acceptance; do not misrepresent code provenance.
