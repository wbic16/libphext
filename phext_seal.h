/*
 * Phext xz seal (libphext 0.6.0) — parity with libphext-rs src/seal.rs
 *
 * Copyright: (c) 2024 Will Bickford
 * License: MIT
 *
 * Compression is a seal, not a working format. A phext is authored,
 * edited, and queried as plain text; when a phext is finished —
 * archived, published, or transported — it may be sealed:
 * xz-compressed as a whole. Unsealing restores the exact original bytes.
 *
 * Seal specification:
 * * The seal is a standard xz stream (liblzma) over the phext's exact
 *   bytes. Decoders in any implementation must accept any valid xz
 *   stream.
 * * This implementation encodes at preset 6 with a CRC64 check. The
 *   preset is an encoder choice, not a format requirement; seals
 *   produced at other presets unseal identically.
 * * Sealing is deterministic for a given encoder and preset.
 *
 * This is the one liblzma-dependent unit of the C port (Will approved
 * the dependency for the 0.6.0 parity release, 2026-11-15). Link with
 * -llzma. The core library (phext.h / phext.c) stays dependency-free.
 */

#include "phext.h"

/* The xz preset this implementation seals with. */
#define PHEXT_SEAL_PRESET 6

/* Return codes: PHEXT_SEAL_OK on success. */
#define PHEXT_SEAL_OK 0
#define PHEXT_SEAL_ERROR -1

/*
phext_seal_bytes: seals raw phext bytes into *out (an owned phext_buffer).
phext_seal: seals a NUL-terminated phext string.
Returns PHEXT_SEAL_OK, or PHEXT_SEAL_ERROR when encoding fails.
*/
int phext_seal_bytes(const unsigned char* input, size_t length, phext_buffer* out);
int phext_seal(const char* input, phext_buffer* out);

/*
phext_unseal: unseals a sealed phext back to its exact original bytes
in *out. Returns PHEXT_SEAL_ERROR when the data is not a valid xz
stream (including empty input).
*/
int phext_unseal(const unsigned char* data, size_t length, phext_buffer* out);

/*
phext_unseal_string: unseals directly to a malloc'd NUL-terminated
string in *out. Returns PHEXT_SEAL_ERROR when the data is not a valid
xz stream, or when the unsealed bytes are not valid UTF-8 (to unseal
foreign content that may carry non-UTF-8 scrolls, phext_unseal to
bytes and pass them through phext_ingest instead).
*/
int phext_unseal_string(const unsigned char* data, size_t length, char** out);
