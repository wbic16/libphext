/*
 * test_phext: 0.6.0 unit tests for the C port — the rs test suites for
 * binary scrolls (src/test_binary.rs) and chains (src/test_chain.rs)
 * ported to C, plus core fetch/insert/replace checks against the
 * reference get_subspace_coordinates semantics.
 * Exits 0 when every check passes; prints FAIL lines otherwise.
 */
#include "phext.h"

#include <stdio.h>
#include <string.h>

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
   checks += 1; \
   if (!(cond)) { failures += 1; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_STR(actual, expected) do { \
   checks += 1; \
   const char* a_ = (actual); \
   if (a_ == 0 || strcmp(a_, expected) != 0) { \
      failures += 1; \
      printf("FAIL %s:%d: got [%s] want [%s]\n", __FILE__, __LINE__, a_ ? a_ : "(null)", expected); \
   } \
} while (0)

static phext_coordinate C(const char* address)
{
   return phext_to_coordinate(address);
}

static int buffer_equals(const phext_buffer* buffer, const unsigned char* expected, size_t length)
{
   return buffer->length == length && (length == 0 || memcmp(buffer->data, expected, length) == 0);
}

/* ------------------------------------------------------------------ */
/* core fetch / insert / replace                                       */
/* ------------------------------------------------------------------ */
static void test_core(void)
{
   char* doc = phext_insert("", C("1.1.1/1.1.1/1.1.1"), "plain text scroll");
   char* fetched = phext_fetch(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(fetched, "plain text scroll");
   free(fetched);

   /* sparse insert: breaks are synthesized up to the target */
   char* doc2 = phext_insert(doc, C("1.1.1/1.1.1/1.1.3"), "third");
   fetched = phext_fetch(doc2, C("1.1.1/1.1.1/1.1.3"));
   CHECK_STR(fetched, "third");
   free(fetched);
   /* the skipped scroll 2 is present-but-empty: fetch returns "" */
   fetched = phext_fetch(doc2, C("1.1.1/1.1.1/1.1.2"));
   CHECK_STR(fetched, "");
   free(fetched);
   /* an unoccupied far coordinate also returns "" */
   fetched = phext_fetch(doc2, C("9.9.9/9.9.9/9.9.9"));
   CHECK_STR(fetched, "");
   free(fetched);

   /* replace overwrites in place */
   char* doc3 = phext_replace(doc2, C("1.1.1/1.1.1/1.1.1"), "replaced");
   fetched = phext_fetch(doc3, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(fetched, "replaced");
   free(fetched);
   fetched = phext_fetch(doc3, C("1.1.1/1.1.1/1.1.3"));
   CHECK_STR(fetched, "third");
   free(fetched);

   /* higher-dimension insert + fetch (mirrors the Makefile corpus) */
   char* doc4 = phext_insert("", C("2.2.2/2.2.2/2.2.5"), "far away");
   fetched = phext_fetch(doc4, C("2.2.2/2.2.2/2.2.5"));
   CHECK_STR(fetched, "far away");
   free(fetched);

   /* legacy entry point keeps its NULL-on-empty contract */
   CHECK(phext_fetch_text(doc, C("9.9.9/9.9.9/9.9.9")) == 0);
   char* legacy = phext_fetch_text(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(legacy, "plain text scroll");
   free(legacy);

   free(doc);
   free(doc2);
   free(doc3);
   free(doc4);
}

/* ------------------------------------------------------------------ */
/* binary scrolls — ported from rs src/test_binary.rs                  */
/* ------------------------------------------------------------------ */
static void test_encode_decode_round_trip(void)
{
   /* empty payload: header line alone */
   char* scroll = phext_encode_scroll(0, 0);
   CHECK_STR(scroll, "phext-binary:1:base64\n");
   phext_buffer decoded;
   CHECK(phext_decode_scroll(scroll, &decoded) == 1);
   CHECK(decoded.length == 0);
   phext_buffer_free(&decoded);
   free(scroll);

   /* every byte value */
   unsigned char all_bytes[256];
   int i;
   for (i = 0; i < 256; ++i) { all_bytes[i] = (unsigned char) i; }
   scroll = phext_encode_scroll(all_bytes, 256);
   CHECK(phext_is_binary_scroll(scroll));
   CHECK(phext_decode_scroll(scroll, &decoded) == 1);
   CHECK(buffer_equals(&decoded, all_bytes, 256));
   phext_buffer_free(&decoded);
   free(scroll);

   /* a plainly non-UTF-8 sequence */
   const unsigned char foreign[] = {0xFF, 0xFE, 0x80, 0x00, 0x61, 0xC3, 0x28};
   scroll = phext_encode_scroll(foreign, sizeof(foreign));
   CHECK(phext_decode_scroll(scroll, &decoded) == 1);
   CHECK(buffer_equals(&decoded, foreign, sizeof(foreign)));
   phext_buffer_free(&decoded);
   free(scroll);

   /* >= 1 MiB payload, deterministic pattern covering all byte values */
   size_t big_length = 1024 * 1024 + 13;
   unsigned char* big = (unsigned char*) malloc(big_length);
   size_t j;
   for (j = 0; j < big_length; ++j) { big[j] = (unsigned char) (j % 256); }
   scroll = phext_encode_scroll(big, big_length);
   CHECK(phext_decode_scroll(scroll, &decoded) == 1);
   CHECK(buffer_equals(&decoded, big, big_length));
   phext_buffer_free(&decoded);
   free(scroll);
   free(big);
}

static void test_decode_scroll_rejects_non_binary(void)
{
   phext_buffer decoded;
   CHECK(phext_decode_scroll("just some text", &decoded) == 0);
   CHECK(phext_decode_scroll("", &decoded) == 0);
   /* header must be a complete line: no line break, no binary scroll */
   CHECK(phext_decode_scroll("phext-binary:1:base64", &decoded) == 0);
   CHECK(!phext_is_binary_scroll("phext-binary:1:base64"));
   /* malformed base64 body: recognisably binary, undecodable */
   const char* malformed = "phext-binary:1:base64\n!!! not base64 !!!";
   CHECK(phext_is_binary_scroll(malformed));
   CHECK(phext_decode_scroll(malformed, &decoded) == 0);
   /* whitespace in the body is not strict base64 */
   CHECK(phext_decode_scroll("phext-binary:1:base64\nQUJD RA==", &decoded) == 0);
   /* missing padding is not strict base64 */
   CHECK(phext_decode_scroll("phext-binary:1:base64\nQUI", &decoded) == 0);
}

static void test_insert_and_fetch_binary(void)
{
   phext_coordinate target = C("1.1.1/1.1.1/1.1.2");
   const unsigned char payload[] = {0x00, 0x01, 0x02, 0xFE, 0xFF, 0x7F, 0x80};

   char* doc = phext_insert("", C("1.1.1/1.1.1/1.1.1"), "plain text scroll");
   char* doc2 = phext_insert_binary(doc, target, payload, sizeof(payload));

   char* fetched = phext_fetch(doc2, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(fetched, "plain text scroll");
   free(fetched);

   phext_buffer bytes = phext_fetch_binary(doc2, target);
   CHECK(buffer_equals(&bytes, payload, sizeof(payload)));
   phext_buffer_free(&bytes);

   char* stored = phext_fetch(doc2, target);
   CHECK(phext_is_binary_scroll(stored));
   phext_buffer decoded;
   CHECK(phext_decode_scroll(stored, &decoded) == 1);
   CHECK(buffer_equals(&decoded, payload, sizeof(payload)));
   phext_buffer_free(&decoded);
   free(stored);
   free(doc);
   free(doc2);
}

static void test_fetch_binary_fallbacks(void)
{
   char* doc = phext_insert("", C("1.1.1/1.1.1/1.1.1"), "hello");
   phext_buffer bytes = phext_fetch_binary(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK(buffer_equals(&bytes, (const unsigned char*) "hello", 5));
   phext_buffer_free(&bytes);

   bytes = phext_fetch_binary(doc, C("9.9.9/9.9.9/9.9.9"));
   CHECK(bytes.length == 0);
   phext_buffer_free(&bytes);
   free(doc);

   /* a malformed binary scroll yields its own bytes verbatim, not a failure */
   const char* malformed = "phext-binary:1:base64\n!!!";
   doc = phext_insert("", C("1.1.1/1.1.1/1.1.1"), malformed);
   bytes = phext_fetch_binary(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK(buffer_equals(&bytes, (const unsigned char*) malformed, strlen(malformed)));
   phext_buffer_free(&bytes);
   free(doc);
}

static void test_update_rule(void)
{
   phext_coordinate target = C("1.1.1/1.1.1/1.1.1");
   /* payload deliberately includes dimension-break bytes (0x17, 0x1C):
      armoured as base64, they round-trip as content, not structure */
   const unsigned char payload[] = {0xFF, 0x17, 0x1C, 0x80, 0x00};

   char* doc = phext_replace_bytes("", target, payload, sizeof(payload));
   char* stored = phext_fetch(doc, target);
   CHECK(phext_is_binary_scroll(stored));
   free(stored);
   phext_buffer bytes = phext_fetch_binary(doc, target);
   CHECK(buffer_equals(&bytes, payload, sizeof(payload)));
   phext_buffer_free(&bytes);
   free(doc);

   doc = phext_insert_bytes("", target, payload, sizeof(payload));
   bytes = phext_fetch_binary(doc, target);
   CHECK(buffer_equals(&bytes, payload, sizeof(payload)));
   phext_buffer_free(&bytes);
   free(doc);

   /* valid UTF-8 stays ordinary text */
   const char* text = "ordinary text \xE2\x9C\x93"; /* "ordinary text ✓" */
   doc = phext_replace_bytes("", target, (const unsigned char*) text, strlen(text));
   char* fetched = phext_fetch(doc, target);
   CHECK_STR(fetched, text);
   CHECK(!phext_is_binary_scroll(fetched));
   free(fetched);
   bytes = phext_fetch_binary(doc, target);
   CHECK(buffer_equals(&bytes, (const unsigned char*) text, strlen(text)));
   phext_buffer_free(&bytes);
   free(doc);
}

static void test_ingest(void)
{
   /* a phext byte stream whose second scroll is not valid UTF-8 */
   unsigned char stream[16];
   size_t n = 0;
   memcpy(stream + n, "hello", 5); n += 5;
   stream[n++] = (unsigned char) PHEXT_SCROLL_BREAK;
   stream[n++] = 0xFF; stream[n++] = 0xFE; stream[n++] = 0x41; stream[n++] = 0x80;

   char* doc = phext_ingest(stream, n);
   char* fetched = phext_fetch(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(fetched, "hello");
   free(fetched);

   phext_coordinate foreign_at = C("1.1.1/1.1.1/1.1.2");
   fetched = phext_fetch(doc, foreign_at);
   CHECK(phext_is_binary_scroll(fetched));
   free(fetched);
   const unsigned char foreign[] = {0xFF, 0xFE, 0x41, 0x80};
   phext_buffer bytes = phext_fetch_binary(doc, foreign_at);
   CHECK(buffer_equals(&bytes, foreign, sizeof(foreign)));
   phext_buffer_free(&bytes);
   free(doc);

   /* a canonical phext ingests as identity */
   char* built = phext_insert("", C("1.1.1/1.1.1/1.1.1"), "first");
   char* built2 = phext_insert(built, C("1.1.1/1.1.1/1.1.2"), "second \xE2\x9C\x93 scroll");
   char* built3 = phext_insert(built2, C("2.1.1/1.3.1/1.1.1"), "far away");
   char* ingested = phext_ingest((const unsigned char*) built3, strlen(built3));
   CHECK_STR(ingested, built3);
   free(built);
   free(built2);
   free(built3);
   free(ingested);

   /* empty and break-only streams: no content, no crash */
   doc = phext_ingest(0, 0);
   fetched = phext_fetch(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(fetched, "");
   free(fetched);
   free(doc);

   const unsigned char breaks[] = {(unsigned char) PHEXT_SCROLL_BREAK, (unsigned char) PHEXT_SECTION_BREAK};
   doc = phext_ingest(breaks, sizeof(breaks));
   fetched = phext_fetch(doc, C("1.1.1/1.1.1/1.1.1"));
   CHECK_STR(fetched, "");
   free(fetched);
   free(doc);
}

static void test_phokenize_bytes_matches(void)
{
   const char* doc = "alpha\x17" "beta\x18" "gamma";
   phext_phoken_list parts = phext_phokenize_bytes((const unsigned char*) doc, strlen(doc));
   CHECK(parts.count == 3);
   if (parts.count == 3)
   {
      CHECK(parts.items[0].length == 5 && memcmp(parts.items[0].data, "alpha", 5) == 0);
      CHECK(parts.items[1].length == 4 && memcmp(parts.items[1].data, "beta", 4) == 0);
      CHECK(parts.items[2].length == 5 && memcmp(parts.items[2].data, "gamma", 5) == 0);
      char* address = phext_get_address(parts.items[2].coord);
      CHECK_STR(address, "1.1.1/1.1.1/1.2.1");
      free(address);
   }
   phext_phoken_list_free(&parts);
}

/* ------------------------------------------------------------------ */
/* chains — ported from rs src/test_chain.rs                           */
/* ------------------------------------------------------------------ */
static void roundtrip(const unsigned char* stream, size_t length)
{
   phext_chain_list chains = phext_parse_chains(stream, length);
   phext_buffer serialized = phext_serialize_chains(&chains);
   checks += 1;
   if (!(serialized.length == length && (length == 0 || memcmp(serialized.data, stream, length) == 0)))
   {
      failures += 1;
      printf("FAIL roundtrip: %zu bytes in, %zu out\n", length, serialized.length);
   }
   phext_buffer_free(&serialized);
   phext_chain_list_free(&chains);
}

#define RT(s) roundtrip((const unsigned char*) (s), strlen(s))

static void test_chains(void)
{
   /* chain-free input is one unclosed chain holding the whole stream */
   const char* doc = "alpha\x17" "beta\x01" "gamma";
   phext_chain_list chains = phext_parse_chains((const unsigned char*) doc, strlen(doc));
   CHECK(chains.count == 1);
   CHECK(chains.chains[0].member_count == 1);
   CHECK(chains.chains[0].closed == 0);
   CHECK(chains.chains[0].members[0].length == strlen(doc));
   phext_chain_list_free(&chains);
   RT(doc);

   /* two phexts chained and closed */
   const char* stream = "edition one\x02" "edition two\x03";
   chains = phext_parse_chains((const unsigned char*) stream, strlen(stream));
   CHECK(chains.count == 1);
   CHECK(chains.chains[0].member_count == 2);
   CHECK(chains.chains[0].closed == 1);
   phext_chain_list_free(&chains);
   RT(stream);

   /* unclosed at EOF */
   chains = phext_parse_chains((const unsigned char*) "one\x02" "two", 7);
   CHECK(chains.count == 1);
   CHECK(chains.chains[0].member_count == 2);
   CHECK(chains.chains[0].closed == 0);
   phext_chain_list_free(&chains);
   RT("one\x02" "two");

   /* two chains in one stream; mixed closed + unclosed */
   chains = phext_parse_chains((const unsigned char*) "a\x02" "b\x03" "c\x02" "d\x03", 8);
   CHECK(chains.count == 2);
   CHECK(chains.chains[0].closed == 1 && chains.chains[1].closed == 1);
   phext_chain_list_free(&chains);
   RT("a\x02" "b\x03" "c\x02" "d\x03");

   chains = phext_parse_chains((const unsigned char*) "a\x03" "b", 3);
   CHECK(chains.count == 2);
   CHECK(chains.chains[0].closed == 1);
   CHECK(chains.chains[1].closed == 0);
   phext_chain_list_free(&chains);
   RT("a\x03" "b");

   /* empty stream: one unclosed chain, one empty member */
   chains = phext_parse_chains((const unsigned char*) "", 0);
   CHECK(chains.count == 1);
   CHECK(chains.chains[0].member_count == 1);
   CHECK(chains.chains[0].members[0].length == 0);
   CHECK(chains.chains[0].closed == 0);
   phext_chain_list_free(&chains);
   RT("");

   /* adjacent chain closes: two closed chains, one empty member each */
   chains = phext_parse_chains((const unsigned char*) "\x03\x03", 2);
   CHECK(chains.count == 2);
   CHECK(chains.chains[0].closed == 1 && chains.chains[0].member_count == 1 && chains.chains[0].members[0].length == 0);
   CHECK(chains.chains[1].closed == 1 && chains.chains[1].member_count == 1 && chains.chains[1].members[0].length == 0);
   phext_chain_list_free(&chains);
   RT("\x03\x03");

   /* empty middle member is preserved */
   chains = phext_parse_chains((const unsigned char*) "a\x02\x02" "b\x03", 5);
   CHECK(chains.count == 1);
   CHECK(chains.chains[0].member_count == 3);
   CHECK(chains.chains[0].members[1].length == 0);
   phext_chain_list_free(&chains);
   RT("a\x02\x02" "b\x03");
}

static void test_multiverse(void)
{
   /* U1: home + scroll 2; U2: home + scroll 3; U3: home only */
   const char* stream =
      "Universe 1 home\x17" "Universe 1 second\x02"
      "Universe 2 home\x17\x17" "Universe 2 third scroll\x02"
      "Universe 3 home\x03";
   phext_chain_list chains = phext_parse_chains((const unsigned char*) stream, strlen(stream));
   CHECK(chains.count == 1);
   const phext_chain* chain = &chains.chains[0];

   char* fetched;
   fetched = phext_chain_fetch(chain, "1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, "Universe 1 home"); free(fetched);
   fetched = phext_chain_fetch(chain, "$1:1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, "Universe 1 home"); free(fetched);
   fetched = phext_chain_fetch(chain, "$2:1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, "Universe 2 home"); free(fetched);
   fetched = phext_chain_fetch(chain, "$3:1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, "Universe 3 home"); free(fetched);
   fetched = phext_chain_fetch(chain, "$2:1.1.1/1.1.1/1.1.3");
   CHECK_STR(fetched, "Universe 2 third scroll"); free(fetched);
   /* universe lacks the scroll */
   fetched = phext_chain_fetch(chain, "$3:1.1.1/1.1.1/1.1.3");
   CHECK_STR(fetched, ""); free(fetched);
   /* universe does not exist */
   fetched = phext_chain_fetch(chain, "$4:1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, ""); free(fetched);
   /* present-but-empty scroll */
   fetched = phext_chain_fetch(chain, "$2:1.1.1/1.1.1/1.1.2");
   CHECK_STR(fetched, ""); free(fetched);
   fetched = phext_chain_fetch(chain, "1.1.1/1.1.1/1.1.2");
   CHECK_STR(fetched, "Universe 1 second"); free(fetched);
   /* malformed universe prefixes yield "" (no error channel) */
   fetched = phext_chain_fetch(chain, "$0:1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, ""); free(fetched);
   fetched = phext_chain_fetch(chain, "$x:1.1.1/1.1.1/1.1.1");
   CHECK_STR(fetched, ""); free(fetched);
   fetched = phext_chain_fetch(chain, "$2");
   CHECK_STR(fetched, ""); free(fetched);

   /* parse_universe_address directly */
   size_t universe = 0;
   phext_coordinate coord;
   CHECK(phext_parse_universe_address("$12:2.1.1/1.1.1/1.1.1", &universe, &coord) == 1);
   CHECK(universe == 12);
   char* address = phext_get_address(coord);
   CHECK_STR(address, "2.1.1/1.1.1/1.1.1");
   free(address);
   CHECK(phext_parse_universe_address("$0:1.1.1/1.1.1/1.1.1", &universe, &coord) == 0);
   CHECK(phext_parse_universe_address("1.1.1/1.1.1/1.1.1", &universe, &coord) == 1 && universe == 1);

   phext_chain_list_free(&chains);
}

int main(void)
{
   test_core();
   test_encode_decode_round_trip();
   test_decode_scroll_rejects_non_binary();
   test_insert_and_fetch_binary();
   test_fetch_binary_fallbacks();
   test_update_rule();
   test_ingest();
   test_phokenize_bytes_matches();
   test_chains();
   test_multiverse();

   printf("test_phext: %d checks, %d failures\n", checks, failures);
   return failures == 0 ? 0 : 1;
}
