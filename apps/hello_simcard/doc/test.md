# hello_simcard Test Results

## Test Environment
- Device: server devkit (ESP32-S3) on ACM0, the only device connected
- microSD module: AMS1117 + 74LVC125A, fed from an external 5V supply with a 47uF buffer capacitor,
  GND common with the devkit; short custom-cut breadboard leads (not dupont wires)
- Wiring: CS=GPIO10, MOSI=GPIO11, SCK=GPIO12, MISO=GPIO13 (SPI2 / FSPI IO_MUX), 10k pull-up on CS
- Cards: `SD16G` (8GB SDHC, FAT32) and `CBADS` (16GB SDHC, FAT32)
- Date: 2026-07-15 (Phase 7b)

## Test Results at 20 MHz (SDMMC_FREQ_DEFAULT)

### Card `SD16G`

| # | Test | Result | Detail |
|---|------|--------|--------|
| 1 | Mount | PASS | Mounted on /sdcard |
| 2 | Card properties | PASS | SDHC, 7684 MB, 512 B sectors, 20000 kHz negotiated |
| 3 | Small file round trip | PASS | 42 bytes written, read back, contents identical |
| 4 | Throughput (256 kB) | PASS | Write 664-672 kB/s, read 478-483 kB/s |
| 5 | Root directory | PASS | HELLO.TXT (42 bytes); the 256 kB speed.bin was cleaned up |
| - | Unmount | PASS | Unmounted cleanly, LED flashes green |

**Summary: 5/5 passed, 0 failed**

### Card `CBADS`

| # | Test | Result | Detail |
|---|------|--------|--------|
| 1 | Mount | PASS | Mounted on /sdcard (MISO probe warned falsely — see below) |
| 2 | Card properties | PASS | SDHC, 15115 MB, 512 B sectors, 20000 kHz negotiated |
| 3 | Small file round trip | PASS | 42 bytes written, read back, contents identical |
| 4 | Throughput (256 kB) | PASS | Write 630 kB/s, read 675 kB/s |
| 5 | Root directory | PASS | HELLO.TXT (42 bytes) |
| - | Unmount | PASS | Unmounted cleanly, LED flashes green |

**Summary: 5/5 passed, 0 failed**

## Speed investigation

| Requested | Actual bus clock | `SD16G` | `CBADS` |
|-----------|------------------|---------|---------|
| 20000 kHz | 20 MHz | PASS | PASS |
| 20001 kHz | 20 MHz | FAIL | not tested |
| 26000 kHz | 20 MHz | FAIL | not tested |
| 40000 kHz | never reached | FAIL (3/3 runs) | FAIL |

Every failure is identical: `sdmmc_enable_hs_mode_and_check: send_csd returned 0x108`, raised while
the bus is still at its 400 kHz probing speed. The 20001 kHz row is the decisive one — same actual
clock as the passing row above it, yet it fails, because the only thing that changed is that the
driver attempted the high-speed mode switch. See `hello_simcard.md` for the full analysis.

## Defects found and fixed during this phase

| Defect | Symptom | Fix |
|--------|---------|-----|
| `DirEntry entries[32]` (~2.3 kB) allocated on the stack | Stack overflow in task main at test 5/5, on a 3584 byte main task stack | Moved the array to the heap |
| Mount failure diagnostic blamed power and wiring unconditionally | At >20 MHz it pointed at the wiring, while the real cause is the high-speed mode switch — actively misleading | Separate message when `requestedFreqKhz > SDMMC_FREQ_DEFAULT`, recommending the 20 MHz fallback |

## Known open issue

The MISO probe reports "MISO reads low ... module has no power or MISO is not connected" for card
`CBADS`, while that card mounts and passes every test. The assumption that a powered card holds MISO
high at idle holds for `SD16G` but not for `CBADS`, so the probe is card-dependent and unreliable.
Left as is for now; it should be softened to a neutral observation rather than a warning.
