/*
 * Phext Buffer Library (libphext)
 *
 * Copyright: (c) 2024 Will Bickford
 * License: MIT
 * 
 * Introduction
 * ------------
 * Phext files are a natural extension of plain text. They add hierarchy
 * via a system of increasingly-larger dimension breaks. These breaks start
 * with normal line breaks (2D) and proceed up to library breaks (11D).
 * 
 * We've annotated the ascii control codes from 0x01 to 0x1f below. Phext
 * has made an effort to remain compatible with a broad swath of software.
 */

#include <stdlib.h>
#include <string.h>

#define PHEXT_VERSION_MAJOR          0
#define PHEXT_VERSION_MINOR          6
#define PHEXT_VERSION_PATCH          1
#define PHEXT_VERSION                "0.6.1" /* 0.6.1: line/column addressing (point, offset, and range addresses); 0.6.0 renumbered to match libphext-rs parity; the 1.0.x line ends (tags v1.0.0/v1.0.1 remain as history) */

#define PHEXT_LIBRARY_BREAK          '\x01' /* 11th dimension: historic: start of heading   */
#define PHEXT_CHAIN_BREAK            '\x02' /* stream level (0.6.0): separates whole phexts within a chain */
#define PHEXT_CHAIN_CLOSE            '\x03' /* stream level (0.6.0): terminates a chain     */
#define HISTORIC_START_OF_TEXT       '\x02' /* start of text - claimed in 0.6.0 as PHEXT_CHAIN_BREAK */
#define HISTORIC_END_OF_TEXT         '\x03' /*     ETX - claimed in 0.6.0 as PHEXT_CHAIN_CLOSE */
#define HISTORIC_END_OF_TRANSMISSION '\x04' /*     EOT - backwards-compatible               */
#define HISTORIC_ENQUERY             '\x05' /* unchanged                                    */
#define HISTORIC_ACK                 '\x06' /* unchanged                                    */
#define PHEXT_MORE_COWBELL           '\x07' /* phext-conforming editors will play cowbell   */
#define HISTORIC_BACKSPACE           '\x08' /* backspace - backwards-compatible             */
#define HISTORIC_HORIZONTAL_TAB      '\x09' /* tab - backwards-compatible                   */
#define PHEXT_LINE_BREAK             '\x0a' /* 2D dimension break - backwards-compatible    */
#define HISTORIC_VERTICAL_TAB        '\x0b' /* backwards-compatible                         */
#define HISTORIC_FORM_FEED           '\x0c' /* form-feed - backwards-compatible             */
#define HISTORIC_CARRIAGE_RETURN     '\x0d' /* carriage return - backwards-compatible       */
#define HISTORIC_SO                  '\x0e' /* unchanged - backwards-compatible             */
#define HISTORIC_SI                  '\x0f' /* unchanged - backwards-compatible             */
#define HISTORIC_DLE                 '\x10' /* unchanged - backwards-compatible             */
#define HISTORIC_DC1                 '\x11' /* unchanged - backwards-compatible             */
#define HISTORIC_DC2                 '\x12' /* unchanged - backwards-compatible             */
#define HISTORIC_DC3                 '\x13' /* unchanged - backwards-compatible             */
#define HISTORIC_DC4                 '\x14' /* unchanged - backwards-compatible             */
#define HISTORIC_NAK                 '\x15' /* unchanged - backwards-compatible             */
#define HISTORIC_SYN                 '\x16' /* unchanged - backwards-compatible             */
#define PHEXT_SCROLL_BREAK           '\x17' /*  3D Break; historic: end of transmission     */
#define PHEXT_SECTION_BREAK          '\x18' /*  4D Break; historic: cancel                  */
#define PHEXT_CHAPTER_BREAK          '\x19' /*  5D Break; historic: end of medium           */
#define PHEXT_BOOK_BREAK             '\x1a' /*  6D Break; historic: substitute              */
#define HISTORIC_ESCAPE              '\x1b' /* unchanged - backwards-compatible             */
#define PHEXT_VOLUME_BREAK           '\x1c' /*  7D Break; historic: file separator          */
#define PHEXT_COLLECTION_BREAK       '\x1d' /*  8D Break; historic: group separator         */
#define PHEXT_SERIES_BREAK           '\x1e' /*  9D Break; historic: record separator        */
#define PHEXT_SHELF_BREAK            '\x1f' /* 10D Break; historic: unit separator          */
#define PHEXT_BUFFER_SIZE            128    /* large enough for truly huge phext addresses: nine 10-digit int components + separators + NUL */


/*
 * Dense graphs: phext is sparse by design. Coordinate components are
 * unbounded, but a fully-populated dense graph contains the product of
 * its dimensions' extents in scrolls — a dense block spanning 1000 in
 * each of 9 dimensions implies 10^27 scrolls. Materializing, exploding,
 * or indexing such a graph costs memory proportional to its populated
 * scroll count. The old 999 cap never protected against this (999^9 was
 * already unreachable); sparsity does. Keep graphs sparse.
 */
typedef struct
{
    int library;
    int shelf;
    int series;
} phext_triplet_z;
typedef struct
{
    int collection;
    int volume;
    int book;
} phext_triplet_y;
typedef struct
{
    int chapter;
    int section;
    int scroll;
} phext_triplet_x;
typedef struct
{
   phext_triplet_z Z;
   phext_triplet_y Y;
   phext_triplet_x X;
} phext_coordinate;

/*
phext_fetch_text: retrieves the plain text string located at the given coordinates.
- phext: a subspace phext to search
- coord: the phext coordinate to locate

note: allocates a new buffer that is null-terminated
*/
char* phext_fetch_text(char* phext, phext_coordinate coord);

/*
phext_initialize_coordinate: Resets the 9D matrix to 1.1.1/1.1.1/1.1.1
- coord: the coordinate to reset
*/
void phext_initialize_coordinate(phext_coordinate* coord);

/*
phext_set_coordinate: translates a phext string address
- coord: the coordinate to reset
- address: text to parse
*/
void phext_set_coordinate(phext_coordinate* coord, char* address);

/*
phext_get_address: produces a quoted string for the given phext address
- coord: the coordinate to translate
*/
char* phext_get_address(phext_coordinate coord);

/*
phext_validate_coordinate: determines if coord points to a valid phext address
- coord: the coordinate to reset
*/
#define PHEXT_NO_ERROR 0
#define PHEXT_INVALID_COORDINATE -1
#define PHEXT_MINIMUM_INDEX 1
/* 0.4.0: PHEXT_MAXIMUM_INDEX was removed (breaking change). Coordinate components are unbounded (native int width); validation only requires each component >= PHEXT_MINIMUM_INDEX. */
int phext_validate_coordinate(phext_coordinate* coord);

/*
break methods - these are all very similar
whenever we break a dimension, we always reset the lower dimensions back to 1
this follows the same pattern as normal text
*/
#define PHEXT_SUCCESS 1
#define PHEXT_FAILURE 0
int phext_scroll_break(phext_coordinate* coord);
int phext_section_break(phext_coordinate* coord);
int phext_chapter_break(phext_coordinate* coord);
int phext_book_break(phext_coordinate* coord);
int phext_volume_break(phext_coordinate* coord);
int phext_collection_break(phext_coordinate* coord);
int phext_series_break(phext_coordinate* coord);
int phext_shelf_break(phext_coordinate* coord);
int phext_library_break(phext_coordinate* coord);

/* ========================================================================
 * 0.6.0 additions — parity with libphext-rs 0.6.0
 * ------------------------------------------------------------------------
 * Canonical binary scrolls (0.5.0), the xz seal (0.5.0, in phext_seal.h —
 * the one liblzma-dependent unit), and chains + multiverse addressing
 * (0.6.0). Semantics mirror the Rust reference exactly:
 *  - phext_fetch returns "" for a missing or empty scroll (the legacy
 *    phext_fetch_text above keeps its historic NULL-on-empty behaviour);
 *  - chain fetch returns "" for misses and present-but-empty alike;
 *  - parse/serialize of chains are byte-exact inverses for every stream.
 * ======================================================================== */

/*
phext_buffer: an owned byte buffer (data + length). data is always
allocated (even when length is 0) and, for text results, NUL-terminated
after length bytes. Release with phext_buffer_free; release returned
char* results with free().
*/
typedef struct
{
   unsigned char* data;
   size_t length;
} phext_buffer;
void phext_buffer_free(phext_buffer* buffer);

/*
phext_to_coordinate: parses a phext string address into a coordinate
(starts from 1.1.1/1.1.1/1.1.1, like the reference to_coordinate).
*/
phext_coordinate phext_to_coordinate(const char* address);

/*
phext_fetch: retrieves the scroll at coord — the reference (rs) fetch.
Returns a malloc'd NUL-terminated string: the scroll text, or "" when
the coordinate is unoccupied or the scroll is empty.
*/
char* phext_fetch(const char* phext, phext_coordinate coord);

/*
phext_fetch_buffer: length-aware form of phext_fetch (result in *out).
Returns PHEXT_SUCCESS, or PHEXT_FAILURE on allocation failure.
*/
int phext_fetch_buffer(const unsigned char* phext, size_t phext_length, phext_coordinate coord, phext_buffer* out);

/*
phext_insert / phext_replace: return a malloc'd new phext with the scroll
at location inserted / replaced (reference insert/replace semantics:
breaks are synthesized from the nearest preceding scroll up to location).
*/
char* phext_insert(const char* phext, phext_coordinate location, const char* scroll);
char* phext_replace(const char* phext, phext_coordinate location, const char* scroll);

/*
Positioned scrolls: the phokenize stream. phext_phokenize_bytes is the
byte-level twin of phext_phokenize (identical segmentation); scroll
bodies are returned uninterpreted, valid UTF-8 or not.
*/
typedef struct
{
   phext_coordinate coord;
   unsigned char* data;
   size_t length;
} phext_positioned_bytes;
typedef struct
{
   phext_positioned_bytes* items;
   size_t count;
} phext_phoken_list;
phext_phoken_list phext_phokenize_bytes(const unsigned char* input, size_t length);
phext_phoken_list phext_phokenize(const char* input);
void phext_phoken_list_free(phext_phoken_list* list);

/* --- canonical binary scrolls (rs src/binary.rs) --- */
#define PHEXT_BINARY_SCROLL_HEADER "phext-binary:1:base64"

/* phext_is_binary_scroll: 1 when the scroll carries the binary header line */
int phext_is_binary_scroll(const char* scroll);

/* phext_is_valid_utf8: strict UTF-8 validation (the ingestion rule's test) */
int phext_is_valid_utf8(const unsigned char* data, size_t length);

/*
phext_encode_scroll: encodes raw bytes as a binary scroll
(header line + RFC 4648 base64, padding required, no wrapping).
Returns a malloc'd NUL-terminated string.
*/
char* phext_encode_scroll(const unsigned char* payload, size_t length);

/*
phext_decode_scroll: decodes a binary scroll into *out.
Returns 1 on success; 0 when the scroll is not a binary scroll or its
body is not strict base64 (the reference's None).
*/
int phext_decode_scroll(const char* scroll, phext_buffer* out);

/*
phext_fetch_binary: the bytes held at target. A well-formed binary scroll
decodes to its payload; any other scroll yields its own bytes (empty for
an unoccupied coordinate).
*/
phext_buffer phext_fetch_binary(const char* phext, phext_coordinate target);

/*
phext_insert_binary / phext_replace_binary: store payload at location as
a binary scroll, unconditionally. Return a malloc'd new phext.
*/
char* phext_insert_binary(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length);
char* phext_replace_binary(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length);

/*
phext_insert_bytes / phext_replace_bytes: byte-oriented entry points
under the ingestion rule — valid UTF-8 is stored as ordinary text;
anything else is stored as a binary scroll holding the base64 of the
exact observed bytes, verbatim. Return a malloc'd new phext.
*/
char* phext_insert_bytes(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length);
char* phext_replace_bytes(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length);

/*
phext_ingest: ingests an externally supplied phext byte stream. Every
scroll is observed as raw bytes: valid UTF-8 passes through verbatim;
a non-UTF-8 scroll is stored as a binary scroll holding the base64 of
its exact bytes. Returns a malloc'd canonical phext (implode of explode).
*/
char* phext_ingest(const unsigned char* bytes, size_t length);

/* --- chains + multiverse addressing (rs src/phext.rs, 0.6.0) --- */

/*
phext_chain: an ordered series of complete phexts (members) joined by
PHEXT_CHAIN_BREAK and optionally terminated by PHEXT_CHAIN_CLOSE.
closed is 1 only when a chain close terminated the chain; a chain that
ends at EOF is unclosed and is never silently promoted to closed.
Members are length-aware buffers (whole phexts, as bytes).
*/
typedef struct
{
   phext_buffer* members;
   size_t member_count;
   int closed;
} phext_chain;
typedef struct
{
   phext_chain* chains;
   size_t count;
} phext_chain_list;

/*
phext_parse_chains: splits a stream into its chains. Empty members and
empty chains are preserved exactly; a stream with no chain bytes is a
single unclosed chain holding one member.
*/
phext_chain_list phext_parse_chains(const unsigned char* input, size_t length);
void phext_chain_list_free(phext_chain_list* list);

/*
phext_serialize_chains / phext_chain_serialize: the byte-exact inverse
of phext_parse_chains (per chain: members joined by PHEXT_CHAIN_BREAK,
plus a trailing PHEXT_CHAIN_CLOSE when closed).
*/
phext_buffer phext_serialize_chains(const phext_chain_list* list);
phext_buffer phext_chain_serialize(const phext_chain* chain);

/*
phext_parse_universe_address: parses "$N:<address>" or a bare <address>
(universe 1). Returns 1 and sets *universe_out / *coord_out on success;
0 when the universe prefix is malformed or $0 (the reference's None).
*/
int phext_parse_universe_address(const char* address, size_t* universe_out, phext_coordinate* coord_out);

/*
phext_chain_fetch: fetches a scroll from one chain by multiverse
address ("$N:coord", or a bare coordinate = universe 1). Returns a
malloc'd NUL-terminated string: the scroll text, or "" for an
out-of-range universe, a missing scroll, a malformed universe prefix,
or a present-but-empty scroll (the reference has no error channel here).
phext_chain_fetch_buffer is the length-aware form.
*/
char* phext_chain_fetch(const phext_chain* chain, const char* address);
int phext_chain_fetch_buffer(const phext_chain* chain, const char* address, phext_buffer* out);

/* --- line/column addressing (rs src/phext.rs, 0.6.1) --- */

/*
phext_scroll_position: a caret boundary inside one scroll — 1-based
line and column. Column 1 is the boundary before the line's first
character; a line of n characters has valid columns 1..n+1. Lines split
on 0x0A; columns count Unicode scalar values, never bytes.
*/
typedef struct
{
   size_t line;
   size_t column;
} phext_scroll_position;

/*
phext_point_address: one parsed point address,
"[$N:]<9D coordinate>[@<line>,<column>[+<offsetChars>]]". universe is
1-based (1 when the "$N:" prefix is absent). has_position / has_offset
mark the optional parts; offset is a forward selection in Unicode
scalar values, counted within the scroll and clipped at its end.
*/
typedef struct
{
   size_t universe;
   phext_coordinate coordinate;
   int has_position;
   phext_scroll_position position;
   int has_offset;
   size_t offset;
} phext_point_address;

/*
phext_range_address: a parsed range — two point endpoints joined by
one hyphen, normalized at parse time to ascending (universe,
coordinate, line, column) order; direction is discarded. An endpoint
without a position names its whole scroll (as start: its first
boundary; as end: its last). Range endpoints never carry an offset.
*/
typedef struct
{
   phext_point_address start;
   phext_point_address end;
} phext_range_address;

/*
phext_parse_point_address / phext_parse_range_address: parse the 0.6.1
address forms. Return 1 and fill *out on success; 0 on any malformed
input (universe 0, a coordinate component / line / column below 1, a
sign anywhere, an offset on a range endpoint, or trailing structure).
"$1:" is accepted on input.
*/
int phext_parse_point_address(const char* address, phext_point_address* out);
int phext_parse_range_address(const char* address, phext_range_address* out);

/*
phext_format_point_address / phext_format_range_address: the canonical
serialization — "$1:" never written, integers in minimal decimal form,
range endpoints ascending. Return malloc'd NUL-terminated strings.
*/
char* phext_format_point_address(const phext_point_address* point);
char* phext_format_range_address(const phext_range_address* range);

/*
phext_chain_resolve_point: resolves a point address against a chain —
(universe, coordinate, character offset within the scroll), the offset
counting Unicode scalar values. A point without a position resolves to
offset 0 of its scroll. Returns 1 on success; 0 when the universe or
scroll is absent or the position is out of range — resolution errors,
never clamps. Any out pointer may be NULL.
*/
int phext_chain_resolve_point(const phext_chain* chain, const phext_point_address* point,
                              size_t* universe_out, phext_coordinate* coord_out, size_t* offset_out);

/*
phext_chain_fetch_point: fetches a point address string. A bare point
is a caret and fetches ""; the offset form fetches the selected
characters (scalars) from the point, terminating at the scroll
boundary. Any parse or resolution failure fetches "". Returns a
malloc'd NUL-terminated string; phext_chain_fetch_point_buffer is the
length-aware form (PHEXT_SUCCESS, with "" in *out on any failure).
*/
char* phext_chain_fetch_point(const phext_chain* chain, const char* address);
int phext_chain_fetch_point_buffer(const phext_chain* chain, const char* address, phext_buffer* out);

/*
phext_range_segment: one covered scroll's segment of a resolved range
— (universe, coordinate, segment text). Segments are returned in
ascending (universe, coordinate) order; endpoint scrolls contribute
partial segments, scrolls strictly between contribute whole segments.
*/
typedef struct
{
   size_t universe;
   phext_coordinate coord;
   phext_buffer text;
} phext_range_segment;
typedef struct
{
   phext_range_segment* items;
   size_t count;
} phext_range_segment_list;
void phext_range_segment_list_free(phext_range_segment_list* list);

/*
phext_chain_resolve_range: resolves a range against a chain into its
per-scroll segments. Returns 1 and fills *out on success; 0 when either
endpoint fails to resolve — an out-of-range endpoint is an error,
never a clamp.
*/
int phext_chain_resolve_range(const phext_chain* chain, const phext_range_address* range,
                              phext_range_segment_list* out);

/*
phext_chain_fetch_range: fetches a range address string — the spanned
text with phext structure bytes preserved, as a phext (chain)
fragment: each covered universe's non-empty segments are inserted at
their coordinates into an empty phext (ascending order), universes are
joined by PHEXT_CHAIN_BREAK, and there is NO trailing
PHEXT_CHAIN_CLOSE. Empty segments contribute nothing; an empty range
and any parse or resolution failure fetch "". Returns a malloc'd
NUL-terminated string; phext_chain_fetch_range_buffer is the
length-aware form (PHEXT_SUCCESS, with "" in *out on any failure).
*/
char* phext_chain_fetch_range(const phext_chain* chain, const char* address);
int phext_chain_fetch_range_buffer(const phext_chain* chain, const char* address, phext_buffer* out);
