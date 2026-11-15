/*
 * test_fixtures: shared libphext 0.6 chain conformance harness.
 *
 *   test_fixtures <fixtures-chains-dir>
 *
 * Consumes the fixture set described in FIXTURES.md (generated from
 * libphext-rs 0.6.0): for every case it verifies the input bytes/sha256,
 * the parsed chain structure (chains, closed flags, member bytes/sha256,
 * per-member scroll counts), every scroll's coordinate/byte-length/sha256
 * against manifest.json, the byte-identical parse->serialize round-trip,
 * and every fetch byte-for-byte. Prints one PASS/FAIL line per case and
 * exits non-zero when any case fails.
 */
#include "phext.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* SHA-256 (FIPS 180-4) — test support only                            */
/* ------------------------------------------------------------------ */
typedef struct
{
   unsigned int state[8];
   unsigned long long total;
   unsigned char block[64];
   size_t used;
} sha256_ctx;

static const unsigned int SHA256_K[64] = {
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(sha256_ctx* ctx, const unsigned char* block)
{
   unsigned int w[64];
   int i;
   for (i = 0; i < 16; ++i)
   {
      w[i] = ((unsigned int) block[i * 4] << 24) | ((unsigned int) block[i * 4 + 1] << 16) |
             ((unsigned int) block[i * 4 + 2] << 8) | (unsigned int) block[i * 4 + 3];
   }
   for (i = 16; i < 64; ++i)
   {
      unsigned int s0 = ROTR(w[i - 15], 7) ^ ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
      unsigned int s1 = ROTR(w[i - 2], 17) ^ ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
   }
   unsigned int a = ctx->state[0], b = ctx->state[1], c = ctx->state[2], d = ctx->state[3];
   unsigned int e = ctx->state[4], f = ctx->state[5], g = ctx->state[6], h = ctx->state[7];
   for (i = 0; i < 64; ++i)
   {
      unsigned int s1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
      unsigned int ch = (e & f) ^ (~e & g);
      unsigned int temp1 = h + s1 + ch + SHA256_K[i] + w[i];
      unsigned int s0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
      unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
      unsigned int temp2 = s0 + maj;
      h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
   }
   ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
   ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(sha256_ctx* ctx)
{
   static const unsigned int initial[8] = {
      0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
   };
   memcpy(ctx->state, initial, sizeof(initial));
   ctx->total = 0;
   ctx->used = 0;
}

static void sha256_update(sha256_ctx* ctx, const unsigned char* data, size_t length)
{
   ctx->total += length;
   while (length > 0)
   {
      size_t take = 64 - ctx->used;
      if (take > length) { take = length; }
      memcpy(ctx->block + ctx->used, data, take);
      ctx->used += take;
      data += take;
      length -= take;
      if (ctx->used == 64)
      {
         sha256_block(ctx, ctx->block);
         ctx->used = 0;
      }
   }
}

static void sha256_final_hex(sha256_ctx* ctx, char* out_hex)
{
   unsigned long long bits = ctx->total * 8;
   unsigned char pad = 0x80;
   sha256_update(ctx, &pad, 1);
   unsigned char zero = 0;
   while (ctx->used != 56) { sha256_update(ctx, &zero, 1); }
   unsigned char length_bytes[8];
   int i;
   for (i = 0; i < 8; ++i) { length_bytes[i] = (unsigned char) (bits >> (56 - i * 8)); }
   /* feed the length without counting it in the total */
   size_t saved_total = (size_t) ctx->total;
   ctx->total -= 8;
   (void) saved_total;
   /* append manually: reuse update but fix total afterwards is unnecessary now */
   {
      sha256_ctx copy = *ctx;
      (void) copy;
   }
   /* simpler: append length bytes through the block machinery directly */
   {
      size_t before = ctx->used;
      memcpy(ctx->block + before, length_bytes, 8);
      ctx->used = 64;
      sha256_block(ctx, ctx->block);
      ctx->used = 0;
   }
   static const char* digits = "0123456789abcdef";
   for (i = 0; i < 8; ++i)
   {
      unsigned int word = ctx->state[i];
      int j;
      for (j = 0; j < 4; ++j)
      {
         unsigned char byte = (unsigned char) (word >> (24 - j * 8));
         out_hex[(i * 4 + j) * 2] = digits[byte >> 4];
         out_hex[(i * 4 + j) * 2 + 1] = digits[byte & 0x0F];
      }
   }
   out_hex[64] = 0;
}

static void sha256_hex(const unsigned char* data, size_t length, char* out_hex)
{
   sha256_ctx ctx;
   sha256_init(&ctx);
   if (length > 0) { sha256_update(&ctx, data, length); }
   sha256_final_hex(&ctx, out_hex);
}

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */
static unsigned char* read_file(const char* path, size_t* length_out)
{
   FILE* fp = fopen(path, "rb");
   if (!fp) { return 0; }
   fseek(fp, 0, SEEK_END);
   long size = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   unsigned char* data = (unsigned char*) malloc(size > 0 ? (size_t) size : 1);
   if (size > 0 && fread(data, 1, (size_t) size, fp) != (size_t) size)
   {
      free(data);
      fclose(fp);
      return 0;
   }
   fclose(fp);
   *length_out = (size_t) size;
   return data;
}

static int failures = 0;
static char first_failure[512];

static void fail(const char* case_name, const char* detail)
{
   if (failures == 0)
   {
      snprintf(first_failure, sizeof(first_failure), "%s", detail);
   }
   failures += 1;
   printf("  FAIL [%s] %s\n", case_name, detail);
}

/* expected data parsed from expected.tsv */
typedef struct
{
   int chain_index;
   int closed;
   int member_count;
} expected_chain;

typedef struct
{
   int chain_index;
   int member_index;
   size_t bytes;
   char sha256[65];
   int scrolls;
} expected_member;

typedef struct
{
   int chain_index;
   char address[256];
   char file[256];
   size_t bytes;
   char sha256[65];
} expected_fetch;

static int field_int(const char* field, const char* key)
{
   /* field is "key=value" */
   const char* eq = strchr(field, '=');
   (void) key;
   return eq ? atoi(eq + 1) : 0;
}

static const char* field_value(const char* field)
{
   const char* eq = strchr(field, '=');
   return eq ? eq + 1 : field;
}

/* split a line into TAB-separated fields (in place) */
static int split_fields(char* line, char** fields, int max_fields)
{
   int count = 0;
   char* cursor = line;
   while (count < max_fields)
   {
      fields[count++] = cursor;
      char* tab = strchr(cursor, '\t');
      if (!tab) { break; }
      *tab = 0;
      cursor = tab + 1;
   }
   return count;
}

/* ------------------------------------------------------------------ */
/* per-case run                                                        */
/* ------------------------------------------------------------------ */
static int run_case(const char* root, const char* case_name)
{
   int case_failures_start = failures;
   char path[1024];

   snprintf(path, sizeof(path), "%s/%s/input.phext", root, case_name);
   size_t input_length = 0;
   unsigned char* input = read_file(path, &input_length);
   if (!input)
   {
      fail(case_name, "cannot read input.phext");
      return 0;
   }

   snprintf(path, sizeof(path), "%s/%s/expected.tsv", root, case_name);
   size_t tsv_length = 0;
   unsigned char* tsv_raw = read_file(path, &tsv_length);
   if (!tsv_raw)
   {
      fail(case_name, "cannot read expected.tsv");
      free(input);
      return 0;
   }
   char* tsv = (char*) malloc(tsv_length + 1);
   memcpy(tsv, tsv_raw, tsv_length);
   tsv[tsv_length] = 0;
   free(tsv_raw);

   size_t expected_input_bytes = 0;
   char expected_input_sha[65] = "";
   int expected_chain_count = -1;
   expected_chain chains_exp[64];
   int chain_exp_count = 0;
   expected_member members_exp[64];
   int member_exp_count = 0;
   expected_fetch fetches_exp[64];
   int fetch_exp_count = 0;

   char* saveptr = 0;
   char* line = strtok_r(tsv, "\n", &saveptr);
   while (line)
   {
      size_t line_length = strlen(line);
      if (line_length > 0 && line[line_length - 1] == '\r') { line[line_length - 1] = 0; }
      char* fields[16];
      int nf = split_fields(line, fields, 16);
      if (nf >= 1)
      {
         if (strcmp(fields[0], "input_bytes") == 0 && nf >= 2)
         {
            expected_input_bytes = (size_t) strtoull(fields[1], 0, 10);
         }
         else if (strcmp(fields[0], "input_sha256") == 0 && nf >= 2)
         {
            snprintf(expected_input_sha, sizeof(expected_input_sha), "%s", fields[1]);
         }
         else if (strcmp(fields[0], "chains") == 0 && nf >= 2)
         {
            expected_chain_count = atoi(fields[1]);
         }
         else if (strcmp(fields[0], "chain") == 0 && nf >= 4)
         {
            expected_chain* entry = &chains_exp[chain_exp_count++];
            entry->chain_index = atoi(fields[1]);
            entry->closed = strstr(fields[2], "true") != 0;
            entry->member_count = field_int(fields[3], "members");
         }
         else if (strcmp(fields[0], "member") == 0 && nf >= 6)
         {
            expected_member* entry = &members_exp[member_exp_count++];
            entry->chain_index = atoi(fields[1]);
            entry->member_index = atoi(fields[2]);
            entry->bytes = (size_t) field_int(fields[3], "bytes");
            snprintf(entry->sha256, sizeof(entry->sha256), "%s", field_value(fields[4]));
            entry->scrolls = field_int(fields[5], "scrolls");
         }
         else if (strcmp(fields[0], "fetch") == 0 && nf >= 7)
         {
            expected_fetch* entry = &fetches_exp[fetch_exp_count++];
            entry->chain_index = field_int(fields[1], "chain");
            snprintf(entry->address, sizeof(entry->address), "%s", field_value(fields[2]));
            snprintf(entry->file, sizeof(entry->file), "%s", field_value(fields[4]));
            entry->bytes = (size_t) field_int(fields[5], "bytes");
            snprintf(entry->sha256, sizeof(entry->sha256), "%s", field_value(fields[6]));
         }
      }
      line = strtok_r(0, "\n", &saveptr);
   }

   /* 1. input bytes + sha256 */
   char hex[65];
   sha256_hex(input, input_length, hex);
   if (input_length != expected_input_bytes)
   {
      fail(case_name, "input byte count mismatch");
   }
   if (strcmp(hex, expected_input_sha) != 0)
   {
      fail(case_name, "input sha256 mismatch");
   }

   /* 2. parse + structure */
   phext_chain_list chains = phext_parse_chains(input, input_length);
   if ((int) chains.count != expected_chain_count)
   {
      fail(case_name, "chain count mismatch");
   }
   int ci;
   for (ci = 0; ci < chain_exp_count && ci < (int) chains.count; ++ci)
   {
      phext_chain* chain = &chains.chains[ci];
      if (chain->closed != chains_exp[ci].closed)
      {
         fail(case_name, "chain closed flag mismatch");
      }
      if ((int) chain->member_count != chains_exp[ci].member_count)
      {
         fail(case_name, "chain member count mismatch");
      }
   }
   int mi;
   for (mi = 0; mi < member_exp_count; ++mi)
   {
      expected_member* exp = &members_exp[mi];
      if (exp->chain_index >= (int) chains.count) { fail(case_name, "member chain index out of range"); continue; }
      phext_chain* chain = &chains.chains[exp->chain_index];
      if (exp->member_index >= (int) chain->member_count) { fail(case_name, "member index out of range"); continue; }
      phext_buffer* member = &chain->members[exp->member_index];
      if (member->length != exp->bytes)
      {
         fail(case_name, "member byte count mismatch");
      }
      sha256_hex(member->data, member->length, hex);
      if (strcmp(hex, exp->sha256) != 0)
      {
         fail(case_name, "member sha256 mismatch");
      }
      phext_phoken_list scrolls = phext_phokenize_bytes(member->data, member->length);
      if ((int) scrolls.count != exp->scrolls)
      {
         char detail[128];
         snprintf(detail, sizeof(detail), "member %d/%d scroll count: got %zu want %d",
                  exp->chain_index, exp->member_index, scrolls.count, exp->scrolls);
         fail(case_name, detail);
      }
      phext_phoken_list_free(&scrolls);
   }

   /* 2b. per-scroll coordinate/length/sha256 from manifest.json, in order */
   snprintf(path, sizeof(path), "%s/%s/manifest.json", root, case_name);
   size_t manifest_length = 0;
   unsigned char* manifest_raw = read_file(path, &manifest_length);
   if (!manifest_raw)
   {
      fail(case_name, "cannot read manifest.json");
   }
   else
   {
      char* manifest = (char*) malloc(manifest_length + 1);
      memcpy(manifest, manifest_raw, manifest_length);
      manifest[manifest_length] = 0;
      free(manifest_raw);

      const char* cursor = manifest;
      int scroll_mismatch = 0;
      for (ci = 0; ci < (int) chains.count && !scroll_mismatch; ++ci)
      {
         phext_chain* chain = &chains.chains[ci];
         size_t mj;
         for (mj = 0; mj < chain->member_count && !scroll_mismatch; ++mj)
         {
            phext_buffer* member = &chain->members[mj];
            phext_phoken_list scrolls = phext_phokenize_bytes(member->data, member->length);
            size_t si;
            for (si = 0; si < scrolls.count; ++si)
            {
               /* scroll objects are {"b": n, "c": "coord", "s": "sha"}:
                  the byte count precedes the coordinate in the raw JSON */
               const char* c_field = strstr(cursor, "\"c\": \"");
               if (!c_field)
               {
                  fail(case_name, "manifest.json: ran out of scroll entries");
                  scroll_mismatch = 1;
                  break;
               }
               const char* b_field = 0;
               const char* scan = cursor;
               while ((scan = strstr(scan, "\"b\": ")) != 0 && scan < c_field)
               {
                  b_field = scan;
                  scan += 5;
               }
               const char* coord_start = c_field + 6;
               const char* coord_end = strchr(coord_start, '"');
               char coord_text[128];
               size_t coord_length = (size_t) (coord_end - coord_start);
               if (coord_length >= sizeof(coord_text)) { coord_length = sizeof(coord_text) - 1; }
               memcpy(coord_text, coord_start, coord_length);
               coord_text[coord_length] = 0;

               const char* s_field = strstr(coord_end, "\"s\": \"");
               if (!b_field || !s_field)
               {
                  fail(case_name, "manifest.json: malformed scroll entry");
                  scroll_mismatch = 1;
                  break;
               }
               size_t expected_b = (size_t) strtoull(b_field + 5, 0, 10);
               char expected_s[65];
               memcpy(expected_s, s_field + 6, 64);
               expected_s[64] = 0;
               cursor = s_field + 6 + 64;

               char* address = phext_get_address(scrolls.items[si].coord);
               if (strcmp(address, coord_text) != 0)
               {
                  char detail[256];
                  snprintf(detail, sizeof(detail), "scroll coordinate: got %s want %s", address, coord_text);
                  fail(case_name, detail);
                  scroll_mismatch = 1;
               }
               free(address);
               if (scrolls.items[si].length != expected_b)
               {
                  fail(case_name, "scroll byte length mismatch vs manifest");
                  scroll_mismatch = 1;
               }
               sha256_hex(scrolls.items[si].data, scrolls.items[si].length, hex);
               if (strcmp(hex, expected_s) != 0)
               {
                  fail(case_name, "scroll sha256 mismatch vs manifest");
                  scroll_mismatch = 1;
               }
            }
            phext_phoken_list_free(&scrolls);
         }
      }
      free(manifest);
   }

   /* 3. round-trip: serialize(parse(input)) must be byte-identical */
   phext_buffer serialized = phext_serialize_chains(&chains);
   if (!(serialized.length == input_length && (input_length == 0 || memcmp(serialized.data, input, input_length) == 0)))
   {
      fail(case_name, "round-trip is not byte-identical");
   }
   phext_buffer_free(&serialized);

   /* 4. fetches */
   int fi;
   for (fi = 0; fi < fetch_exp_count; ++fi)
   {
      expected_fetch* exp = &fetches_exp[fi];
      snprintf(path, sizeof(path), "%s/%s/%s", root, case_name, exp->file);
      size_t expected_length = 0;
      unsigned char* expected = read_file(path, &expected_length);
      if (!expected)
      {
         fail(case_name, "cannot read fetch expectation file");
         continue;
      }
      if (expected_length != exp->bytes)
      {
         fail(case_name, "fetch expectation file size disagrees with expected.tsv");
      }
      if (exp->chain_index >= (int) chains.count)
      {
         fail(case_name, "fetch chain index out of range");
         free(expected);
         continue;
      }
      phext_buffer out;
      phext_chain_fetch_buffer(&chains.chains[exp->chain_index], exp->address, &out);
      if (!(out.length == expected_length && (expected_length == 0 || memcmp(out.data, expected, expected_length) == 0)))
      {
         char detail[320];
         snprintf(detail, sizeof(detail), "fetch %s: got %zu bytes want %zu", exp->address, out.length, expected_length);
         fail(case_name, detail);
      }
      sha256_hex(out.data, out.length, hex);
      if (strcmp(hex, exp->sha256) != 0)
      {
         fail(case_name, "fetch sha256 mismatch");
      }
      phext_buffer_free(&out);
      free(expected);
   }

   phext_chain_list_free(&chains);
   free(tsv);
   free(input);

   int passed = failures == case_failures_start;
   printf("%s %s (%d fetches)\n", passed ? "PASS" : "FAIL", case_name, fetch_exp_count);
   return passed;
}

int main(int argc, char** argv)
{
   if (argc != 2)
   {
      fprintf(stderr, "usage: test_fixtures <fixtures-chains-dir>\n");
      return 2;
   }
   const char* root = argv[1];

   /* SHA-256 self-check against known vectors before trusting it */
   char hex[65];
   sha256_hex((const unsigned char*) "", 0, hex);
   if (strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") != 0)
   {
      printf("FAIL sha256 self-check (empty): %s\n", hex);
      return 1;
   }
   sha256_hex((const unsigned char*) "abc", 3, hex);
   if (strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") != 0)
   {
      printf("FAIL sha256 self-check (abc): %s\n", hex);
      return 1;
   }

   char path[1024];
   snprintf(path, sizeof(path), "%s/cases.tsv", root);
   size_t cases_length = 0;
   unsigned char* cases_raw = read_file(path, &cases_length);
   if (!cases_raw)
   {
      fprintf(stderr, "cannot read %s\n", path);
      return 1;
   }
   char* cases = (char*) malloc(cases_length + 1);
   memcpy(cases, cases_raw, cases_length);
   cases[cases_length] = 0;
   free(cases_raw);

   int total = 0;
   int passed = 0;
   char* saveptr = 0;
   char* line = strtok_r(cases, "\n", &saveptr);
   int first_line = 1;
   while (line)
   {
      if (!first_line)
      {
         char* tab = strchr(line, '\t');
         if (tab) { *tab = 0; }
         size_t len = strlen(line);
         if (len > 0 && line[len - 1] == '\r') { line[len - 1] = 0; }
         if (strlen(line) > 0)
         {
            total += 1;
            if (run_case(root, line)) { passed += 1; }
         }
      }
      first_line = 0;
      line = strtok_r(0, "\n", &saveptr);
   }
   free(cases);

   printf("test_fixtures: %d/%d cases passed, %d failed checks\n", passed, total, failures);
   return passed == total ? 0 : 1;
}
