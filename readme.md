libphext
--------
libphext is a C library that provides a reference parser for fetching text nodes from phext buffers. Support for writing directly to subspace coordinates will arrive after hello-phext (a rust implementation) is completed.

Version 0.6.0
-------------
Full parity with libphext-rs 0.6.0. This release is renumbered to match the Rust reference: the C port's own 1.0.x line ends here (tags `v1.0.0` / `v1.0.1` remain as history), and the parity release is `0.6.0` (`PHEXT_VERSION` in phext.h).
* Canonical binary scrolls (0.5.0): a binary scroll's content is exactly `phext-binary:1:base64` + a line break + the RFC 4648 base64 body (padding required, no wrapping). `phext_encode_scroll` / `phext_decode_scroll` / `phext_is_binary_scroll`, `phext_insert_binary` / `phext_replace_binary` / `phext_fetch_binary`, and the byte entry points `phext_insert_bytes` / `phext_replace_bytes` / `phext_ingest`, which apply the ingestion rule: valid UTF-8 is stored as ordinary text; anything else is stored as a binary scroll holding the base64 of the exact observed bytes, verbatim.
* The xz seal (0.5.0): `phext_seal` / `phext_seal_bytes` / `phext_unseal` / `phext_unseal_string` in phext_seal.h / phext_seal.c — a standard xz stream (liblzma, preset 6, CRC64) over the phext's exact bytes. This is the port's one library dependency (`-llzma`); the core library stays dependency-free.
* Chains + multiverse addressing (0.6.0): `PHEXT_CHAIN_BREAK` (0x02) separates whole phexts within a chain; `PHEXT_CHAIN_CLOSE` (0x03) terminates a chain (a chain ending at EOF is unclosed). `phext_parse_chains` / `phext_serialize_chains` are byte-exact inverses for every stream. Inside a chain, members are universes indexed from 1: `phext_chain_fetch` resolves `$N:<address>` (a bare address means universe 1) and returns "" for misses and present-but-empty scrolls alike. Conformance: the shared fixture set generated from libphext-rs 0.6.0 lives in tests/fixtures/chains and runs via `make test` (test_fixtures: 10 cases, 42 fetches, byte-identical round-trips).
* Reference fetch/insert/replace: `phext_fetch` returns "" for a missing or empty scroll (the historic `phext_fetch_text` keeps returning NULL there), and `phext_insert` / `phext_replace` follow the reference subspace semantics, synthesizing breaks up to sparse coordinates. `phext_phokenize` / `phext_phokenize_bytes` expose the positioned-scroll stream.

Version 0.4.0
-------------
Coordinate components are now unbounded (native `int` width). The 999 cap is gone:
* `PHEXT_MAXIMUM_INDEX` was removed from phext.h — a breaking change for any code that referenced it.
* Coordinate advance (`phext_*_break`) no longer saturates at 999; it fails only at INT_MAX, where incrementing would overflow.
* `phext_validate_coordinate` is unchanged in effect: it has only ever required each component to be >= 1, and still does.
* The address parser in `phext_set_coordinate` was widened (8 -> 16 chars per component) so full int-width components parse exactly, and `PHEXT_BUFFER_SIZE` grew 64 -> 128 so `phext_get_address` can format nine 10-digit components.

Dense graphs: phext is sparse by design. Coordinate components are unbounded, but a fully-populated dense graph contains the product of its dimensions' extents in scrolls — a dense block spanning 1000 in each of 9 dimensions implies 10^27 scrolls. Materializing, exploding, or indexing such a graph costs memory proportional to its populated scroll count. The old 999 cap never protected against this (999^9 was already unreachable); sparsity does. Keep graphs sparse.

Development and Release
-----------------------
Prerequisites: gcc and make (the Makefile toolchain).

Test:
  1. Run `scripts/test.sh`
  2. That runs exactly: `make test` -- builds phextio + phesql, runs the
     regression fetches against the sample phexts, then runs the 0.6.0 suites
     (test_phext, test_seal, and the shared chain fixtures via test_fixtures).
     The seal tests require the liblzma development files (liblzma-dev).

Build:
  1. Run `scripts/build.sh`
  2. That runs exactly: `make clean && make` -- a clean rebuild of phextio.

Publish:
  There is NO package registry for this library. Distribution is a git tag
  on the GitHub repository, and the script never pushes anything itself.
  1. Run `scripts/publish.sh` -- it runs the tests, rebuilds, prints exactly
     what will be published (git tag `v0.6.0`), and stops.
  2. Only when that output looks right, run `PUBLISH_CONFIRM=1 scripts/publish.sh`
     -- that creates the LOCAL tag `v0.6.0` and prints the push command.
  3. A human then publishes the tag by running exactly: `git push origin v0.6.0`

Files
-----
* main.c: a reference parser named 'phextio' after phext.io
* Makefile: gcc toolchain compatible
  * `make test`: Run regression tests against sample.phext
  * `make`: compiles phext.o and links phextio
* phext.h: Header definitions
  * `phext_coordinate`: encodes the standard Library.Shelf.Series/Collection.Volume.Book/Chapter.Section.Scroll Coordinate
  * `phext_fetch_text`: produces the contents of the text file at the given coordinates within the phext buffer - allocates a new null-terminated buffer that you need to manage
  * `phext_initialize_coordinate`: Sets your coordinates to 1.1.1/1.1.1/1.1.1
  * `phext_set_coordinate`: Sets your coordinates to match a given string representation
  * `phext_get_address`: translates a phext coordinate back to dotted-triplet-triplet form
  * `phext_validate_coordinate`: used to confirm whether or not a coordinate is realistic
  * `phext_scroll_break`: manages your coordinate for a SCROLL_BREAK
  * `phext_section_break`: manages your coordinate for a SECTION_BREAK
  * `phext_chapter_break`: manages your coordinate for a CHAPTER_BREAK
  * `phext_book_break`: manages your coordinate for a BOOK_BREAK  
  * `phext_volume_break`: manages your coordinate for a VOLUME_BREAK
  * `phext_collection_break`: manages your coordinate for a COLLECTION_BREAK
  * `phext_series_break`: manages your coordinate for a SERIES_BREAK
  * `phext_shelf_break`: manages your coordinate for a SHELF_BREAK
  * `phext_library_break`: manages your coordinate for a LIBRARY_BREAK
* phext.c: Reference Parser Implementation (core + binary scrolls + chains)
* phext_seal.h / phext_seal.c: the xz seal (links liblzma; see Version 0.6.0 above)
* test_phext.c / test_seal.c / test_fixtures.c: the 0.6.0 test suites; seal_tool.c is a small cross-check tool for verifying seals against other ports
* tests/fixtures/chains: the shared 0.6 chain conformance fixtures (generated from libphext-rs 0.6.0; see FIXTURES.md in the libphext-rs repo)
