# Wired Apex Pro Mini GG protocol integration

Source research: https://github.com/newnetmp3/ApexProMini_gg_rgb (see docs/PROTOCOL.md and docs/PRESET_COVERAGE.md).

Hardware: USB 1038:161e; HID interface 1; vendor usage page 0xFFC0; usage 1; report ID 0. Verified direct RGB uses a 513-byte HID feature report with bytes `00 40 NN` followed by up to 68 `[USB HID keycode, R, G, B]` tuples and zero padding. Verified clear uses a 65-byte output report beginning `00 41`. The research repo reports physical confirmation of A red, S green, full-board blue, and clear restoring onboard lighting.

This branch introduces detection and Gen2 command selection as a first integration step. It is NOT yet an accepted full keyboard implementation: validate the OpenRGB key array, 68-key layout and RGB ordering, report length mapping, initialization/brightness reads, deinitialization semantics and controller update behavior against the research driver. Do not claim hardware support until a real keyboard passes acceptance tests. In particular, the Mini clear command must be 0x41 and must not inherit an unrelated reset command. Avoid running both the independent apex-mini-ripple.service and OpenRGB as simultaneous lighting writers.

Follow-up: port verified mapping and packet transport, add serializer tests and hardware smoke-test instructions, then assess 31 historical GG Prism identifiers and 14 independent software effects. GG reconstructed presets are not exact GG shader parity; reactive effects require an input-event source and software effect infrastructure rather than an onboard lighting-mode claim. No proprietary SteelSeries descriptors or resources should be copied into this repository.
