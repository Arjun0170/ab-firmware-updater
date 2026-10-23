# A/B firmware updater with simulated flash

A software-only Embedded C model of a two-slot firmware update mechanism. It
installs a local image buffer into simulated NOR-like flash, checks the written
bytes with CRC-32, and writes a commit marker last. A simulated reset then
selects the highest-version valid image. No board, boot transport, or
communications protocol is involved.

## What it demonstrates

- Explicit flash layout and little-endian metadata serialization.
- Inactive-slot programming so an existing valid image remains bootable.
- NOR-style `1 -> 0` programming constraints in the flash model.
- Readback CRC-32 verification and a commit marker written after the image.
- Boot-time image validation and fallback when a newer slot is corrupted.
- Fault injection at every byte boundary of a 300-byte update.

## Run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/updater_sim
```

The simulator installs version 1, interrupts version 2 during programming,
restarts, completes version 2, then flips an image bit. Its output shows the
boot choice at each stage.

## Flash layout

Each of two 4096-byte slots uses the same format:

| Offset | Size | Content |
| --- | ---: | --- |
| 0 | 4 | Magic `FWUP` (little-endian integer) |
| 4 | 4 | Version |
| 8 | 4 | Image length |
| 12 | 4 | Image CRC-32 (IEEE reflected) |
| 16 | 4 | CRC-32 of metadata bytes 0–15 |
| 20 | 4 | Commit marker `COMT` |
| 24 | 8 | Reserved |
| 32 | Up to 4064 | Image bytes |

The 24-byte metadata is written only after image readback. The four-byte commit
marker is the final write. A partial marker cannot equal `COMT`, so boot
selection ignores an interrupted update. Every candidate's metadata and image
are also checked by CRC at boot.

## Code map

| Path | Purpose |
| --- | --- |
| `include/updater.h`, `src/updater.c` | Slot selection, install sequence, CRC, validation |
| `sim/flash_model.*` | Two flash slots, erase/program rules, write fault budget |
| `sim/main.c` | Reset, interrupted install, corruption demonstration |
| `tests/test_main.c` | CRC vector, versions, 324 power-loss boundaries, fallback |
| `docs/design.md` | State and failure reasoning |
| `docs/test-plan.md` | Reproducible test cases |

## Scope and limitations

This is an educational reliability model, not a production bootloader. CRC-32
detects accidental corruption but provides no authenticity. A secure product
would need signed images, a protected trust anchor, a board-specific linker and
vector-table design, actual flash timing and alignment handling, and a runtime
health/rollback policy. The version field is a selection rule, not secure
anti-rollback. The input image must be held outside the target slot. No
physical hardware was used or validated.

License: MIT. See `LICENSE`.
