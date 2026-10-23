# Test plan

Run the commands in the README. `ctest` must report one passing suite.

| Case | Procedure | Expected |
| --- | --- | --- |
| CRC vector | CRC-32 of ASCII `123456789` | `0xCBF43926` |
| Empty flash | Select boot before install | No image selected |
| Version monotonicity | Install same or lower version | Rejected |
| Invalid length | Empty or oversized image | Rejected without flash write |
| Interrupted install | Fail each write byte before the final marker | Earlier image remains selected after reset |
| Completed install | Allow all writes | New slot selected |
| Corruption fallback | Flip one byte in newer image | Earlier valid slot selected |
| Metadata integrity | Flip a version bit in newer metadata | Earlier valid slot selected |
| Slot reuse | Install a third version | Inactive slot reused and newest version selected |

The exhaustive interruption loop covers 300 image bytes, 20 metadata bytes,
and 4 commit-marker bytes: 324 failure positions. The flash model can only
partially fail a program write, not erase. Erase acts on the inactive slot, so
an erase failure would preserve the active slot as well.
