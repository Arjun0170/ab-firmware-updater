# Design notes

## Install sequence

1. Validate input length and find the highest-version bootable slot.
2. Reject a version that does not exceed the selected image's version.
3. Erase the other slot. If there is no valid image, use slot 0.
4. Program the local image in blocks of at most 128 bytes.
5. Read back every byte and compare its CRC-32 to the source image's CRC-32.
6. Write metadata, then write the four-byte commit marker last.

The flash abstraction allows partial writes followed by failure. The model
enforces that programming can only clear bits. An interrupted install never
erases the active slot. Boot selection validates both slots independently and
chooses the highest-version valid image.

## Recovery invariant

Assume an already valid slot A and an install into slot B. Until the final
commit marker write completes, B is ignored. If it completes, B still must
pass a full-image CRC check. Therefore an interruption during any write leaves
at least A bootable under the model's assumptions. A new install can simply
erase and retry the inactive slot; transfer resume is outside this project's
scope.

CRC does not detect malicious changes. Also, this model does not detect an
application that boots but fails during runtime. A production system would
need signed images and an explicit confirmation/rollback state machine.
