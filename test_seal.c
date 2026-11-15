/*
 * test_seal: 0.6.0 xz seal tests for the C port — ported from rs
 * src/test_seal.rs. Exits 0 when every check passes.
 */
#include "phext_seal.h"

#include <stdio.h>
#include <string.h>

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
   checks += 1; \
   if (!(cond)) { failures += 1; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

static void round_trip(const char* doc)
{
   phext_buffer sealed;
   CHECK(phext_seal(doc, &sealed) == PHEXT_SEAL_OK);
   char* restored = 0;
   CHECK(phext_unseal_string(sealed.data, sealed.length, &restored) == PHEXT_SEAL_OK);
   if (restored)
   {
      checks += 1;
      if (strcmp(restored, doc) != 0)
      {
         failures += 1;
         printf("FAIL round trip mismatch for [%s]\n", doc);
      }
      free(restored);
   }
   phext_buffer_free(&sealed);
}

int main(void)
{
   /* round trip over a multi-scroll phext (the sample corpus shape) */
   const char* corpus = "cover text\x17" "second scroll\x18" "new section\x01" "new library";
   phext_buffer sealed;
   CHECK(phext_seal(corpus, &sealed) == PHEXT_SEAL_OK);
   CHECK(sealed.length > 0);
   /* an xz stream starts with the xz magic bytes FD 37 7A 58 5A 00 */
   const unsigned char magic[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
   CHECK(sealed.length >= 6 && memcmp(sealed.data, magic, 6) == 0);
   phext_buffer restored_bytes;
   CHECK(phext_unseal(sealed.data, sealed.length, &restored_bytes) == PHEXT_SEAL_OK);
   CHECK(restored_bytes.length == strlen(corpus) && memcmp(restored_bytes.data, corpus, strlen(corpus)) == 0);
   phext_buffer_free(&restored_bytes);
   phext_buffer_free(&sealed);

   /* empty and small inputs */
   round_trip("");
   round_trip("hello phext");

   /* a phext holding base64 binary scrolls seals with every scroll intact */
   round_trip("cover text\x17" "phext-binary:1:base64\nAAEC/f7/fw==\x17" "after");

   /* sealing is deterministic */
   const char* doc = "the same phext, sealed twice\x17" "the same phext, sealed twice";
   phext_buffer first;
   phext_buffer second;
   CHECK(phext_seal(doc, &first) == PHEXT_SEAL_OK);
   CHECK(phext_seal(doc, &second) == PHEXT_SEAL_OK);
   CHECK(first.length == second.length && memcmp(first.data, second.data, first.length) == 0);
   phext_buffer_free(&first);
   phext_buffer_free(&second);

   /* non-xz data is rejected */
   phext_buffer out;
   const char* not_xz = "this is not an xz stream";
   CHECK(phext_unseal((const unsigned char*) not_xz, strlen(not_xz), &out) == PHEXT_SEAL_ERROR);
   CHECK(phext_unseal((const unsigned char*) "", 0, &out) == PHEXT_SEAL_ERROR);
   char* text = 0;
   const char* plain = "phext, but unsealed";
   CHECK(phext_unseal_string((const unsigned char*) plain, strlen(plain), &text) == PHEXT_SEAL_ERROR);

   /* raw byte round trip, including non-UTF-8 bytes */
   const unsigned char raw[] = {0x00, 0xFF, 0x17, 0x80, 0x61, 0x00};
   CHECK(phext_seal_bytes(raw, sizeof(raw), &sealed) == PHEXT_SEAL_OK);
   CHECK(phext_unseal(sealed.data, sealed.length, &out) == PHEXT_SEAL_OK);
   CHECK(out.length == sizeof(raw) && memcmp(out.data, raw, sizeof(raw)) == 0);
   phext_buffer_free(&out);
   phext_buffer_free(&sealed);
   /* ...but unseal_string refuses non-UTF-8 content */
   CHECK(phext_seal_bytes(raw, sizeof(raw), &sealed) == PHEXT_SEAL_OK);
   CHECK(phext_unseal_string(sealed.data, sealed.length, &text) == PHEXT_SEAL_ERROR);
   phext_buffer_free(&sealed);

   printf("test_seal: %d checks, %d failures\n", checks, failures);
   return failures == 0 ? 0 : 1;
}
