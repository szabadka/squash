# lz4 Plugin #

LZ4 is a small, fast compression library.

For more information about LZ4, see
https://cyan4973.github.io/lz4/

## Codecs ##

- **lz4** — Framed LZ4 data (compatible with lz4 CLI tool)
- **lz4-raw** — Raw LZ4 data.

## Options ##

### lz4 ###

- **level** (integer, 0 - 16, default 0) — higher = better
  compression ratio but slower compression speed
- **block-size** (enum, default 4) — input block size
  - 4 — 64 KiB
  - 5 — 256 KiB
  - 6 — 1 MiB
  - 7 — 4 MiB
- **checksum** (boolean, default false) — whether or not to include a
  checksum (xxHash) for verification

### lz4-raw ###

- **level** (integer, 1-14, default 7) — higher level corresponds to
  better compression ratio but slower compression speed.
  - **1** — LZ4 fast mode (LZ4_compress_fast) acceleration 32
  - **2** — LZ4 fast mode acceleration 24
  - **3** — LZ4 fast mode acceleration 17
  - **4** — LZ4 fast mode acceleration 8
  - **5** — LZ4 fast mode acceleration 4
  - **6** — LZ4 fast mode acceleration 2
  - **7** — The default algorithm (LZ4_compress)
  - **8** — LZ4HC level 2
  - **9** — LZ4HC level 4
  - **10** — LZ4HC level 6
  - **11** — LZ4HC level 9
  - **12** — LZ4HC level 12
  - **13** — LZ4HC level 14
  - **14** — LZ4HC level 16

## License ##

The lz4 plugin is licensed under the [MIT
License](http://opensource.org/licenses/MIT), and LZ4 is licensed
under a [3-clause BSD](http://opensource.org/licenses/BSD-3-Clause)
license.
