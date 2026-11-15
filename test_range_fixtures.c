/*
 * test_range_fixtures: shared libphext 0.6.1 range conformance harness.
 *
 *   test_range_fixtures <fixtures-ranges-dir>
 *
 * Consumes the fixture set described in FIXTURES-RANGES.md (generated
 * from libphext-rs 0.6.1): for every case it parses input.phext with
 * phext_parse_chains, takes chain 0, and runs every op in expected.tsv
 * against the 0.6.1 addressing API (phext_parse_point_address /
 * phext_parse_range_address / phext_chain_resolve_point /
 * phext_chain_resolve_range / phext_chain_fetch_point /
 * phext_chain_fetch_range), comparing canonical forms, resolutions,
 * resolved flags, and fetched bytes exactly. Prints one PASS/FAIL line
 * per case and exits non-zero when any case fails.
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

/* ------------------------------------------------------------------ */
/* file helpers                                                        */
/* ------------------------------------------------------------------ */
static unsigned char* read_file(const char* path, size_t* length_out)
{
   FILE* file = fopen(path, "rb");
   unsigned char* data;
   long size;
   if (file == 0) { return 0; }
   fseek(file, 0, SEEK_END);
   size = ftell(file);
   fseek(file, 0, SEEK_SET);
   data = (unsigned char*) malloc((size_t) size + 1);
   if (size > 0 && fread(data, 1, (size_t) size, file) != (size_t) size)
   {
      fclose(file);
      free(data);
      return 0;
   }
   fclose(file);
   data[size] = 0;
   *length_out = (size_t) size;
   return data;
}

/* split a line into at most max_fields TAB-separated fields, in place;
   returns the field count (trailing newline/CR stripped first) */
static int split_tabs(char* line, char** fields, int max_fields)
{
   int count = 0;
   char* p = line;
   size_t len = strlen(line);
   while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) { line[--len] = 0; }
   while (count < max_fields)
   {
      char* tab;
      fields[count++] = p;
      tab = strchr(p, '\t');
      if (tab == 0) { break; }
      *tab = 0;
      p = tab + 1;
   }
   return count;
}

static const char* field_value(char** fields, int count, const char* key)
{
   int i;
   size_t key_length = strlen(key);
   for (i = 0; i < count; ++i)
   {
      if (strncmp(fields[i], key, key_length) == 0 && fields[i][key_length] == '=')
      {
         return fields[i] + key_length + 1;
      }
   }
   return 0;
}

/* ------------------------------------------------------------------ */
/* one case                                                            */
/* ------------------------------------------------------------------ */
static int run_case(const char* root, const char* case_name)
{
   char path[1024];
   unsigned char* input;
   size_t input_length = 0;
   phext_chain_list chains;
   const phext_chain* chain;
   FILE* tsv;
   char line[8192];
   int ops = 0;
   int failures_before = failures;

   snprintf(path, sizeof(path), "%s/%s/input.phext", root, case_name);
   input = read_file(path, &input_length);
   CHECK(input != 0);
   if (input == 0) { return 0; }
   chains = phext_parse_chains(input, input_length);
   free(input);
   CHECK(chains.count >= 1);
   if (chains.count < 1) { phext_chain_list_free(&chains); return 0; }
   chain = &chains.chains[0];

   snprintf(path, sizeof(path), "%s/%s/expected.tsv", root, case_name);
   tsv = fopen(path, "rb");
   CHECK(tsv != 0);
   if (tsv == 0) { phext_chain_list_free(&chains); return 0; }

   while (fgets(line, sizeof(line), tsv) != 0)
   {
      char* fields[8];
      int count = split_tabs(line, fields, 8);
      const char* kind;
      const char* address;
      if (count < 3 || strcmp(fields[0], "op") != 0) { continue; }
      kind = fields[1];
      address = fields[2];
      ops += 1;

      if (strcmp(kind, "parse_point") == 0)
      {
         int expect_error = strcmp(field_value(fields, count, "error"), "true") == 0;
         phext_point_address point;
         int ok = phext_parse_point_address(address, &point);
         CHECK(ok == !expect_error);
         if (ok && !expect_error)
         {
            char* canonical = phext_format_point_address(&point);
            CHECK(strcmp(canonical, field_value(fields, count, "canonical")) == 0);
            free(canonical);
         }
      }
      else if (strcmp(kind, "parse_range") == 0)
      {
         int expect_error = strcmp(field_value(fields, count, "error"), "true") == 0;
         phext_range_address range;
         int ok = phext_parse_range_address(address, &range);
         CHECK(ok == !expect_error);
         if (ok && !expect_error)
         {
            char* canonical = phext_format_range_address(&range);
            CHECK(strcmp(canonical, field_value(fields, count, "canonical")) == 0);
            free(canonical);
         }
      }
      else if (strcmp(kind, "resolve_point") == 0)
      {
         int expect_error = strcmp(field_value(fields, count, "error"), "true") == 0;
         phext_point_address point;
         size_t universe = 0;
         size_t offset = 0;
         phext_coordinate coord;
         int parsed = phext_parse_point_address(address, &point);
         int ok;
         CHECK(parsed == 1);
         ok = parsed && phext_chain_resolve_point(chain, &point, &universe, &coord, &offset);
         CHECK(ok == !expect_error);
         if (ok && !expect_error)
         {
            char* coord_text = phext_get_address(coord);
            char resolved[1024];
            snprintf(resolved, sizeof(resolved), "%zu|%s|%zu", universe, coord_text, offset);
            free(coord_text);
            CHECK(strcmp(resolved, field_value(fields, count, "resolve")) == 0);
         }
      }
      else if (strcmp(kind, "fetch_point") == 0)
      {
         int expect_resolved = strcmp(field_value(fields, count, "resolved"), "true") == 0;
         phext_point_address point;
         int resolved = phext_parse_point_address(address, &point) &&
                        phext_chain_resolve_point(chain, &point, 0, 0, 0);
         phext_buffer actual;
         unsigned char* expected;
         size_t expected_length = 0;
         CHECK(resolved == expect_resolved);
         snprintf(path, sizeof(path), "%s/%s/%s", root, case_name, field_value(fields, count, "file"));
         expected = read_file(path, &expected_length);
         CHECK(expected != 0);
         phext_chain_fetch_point_buffer(chain, address, &actual);
         CHECK(actual.length == expected_length);
         if (actual.length == expected_length)
         {
            CHECK(memcmp(actual.data, expected, expected_length) == 0);
         }
         phext_buffer_free(&actual);
         free(expected);
      }
      else if (strcmp(kind, "fetch_range") == 0)
      {
         int expect_resolved = strcmp(field_value(fields, count, "resolved"), "true") == 0;
         phext_range_address range;
         int resolved = 0;
         phext_buffer actual;
         unsigned char* expected;
         size_t expected_length = 0;
         if (phext_parse_range_address(address, &range))
         {
            phext_range_segment_list segments;
            resolved = phext_chain_resolve_range(chain, &range, &segments);
            if (resolved) { phext_range_segment_list_free(&segments); }
         }
         CHECK(resolved == expect_resolved);
         snprintf(path, sizeof(path), "%s/%s/%s", root, case_name, field_value(fields, count, "file"));
         expected = read_file(path, &expected_length);
         CHECK(expected != 0);
         phext_chain_fetch_range_buffer(chain, address, &actual);
         CHECK(actual.length == expected_length);
         if (actual.length == expected_length)
         {
            CHECK(memcmp(actual.data, expected, expected_length) == 0);
         }
         phext_buffer_free(&actual);
         free(expected);
      }
      else
      {
         CHECK(0 && "unknown op kind");
      }
   }
   fclose(tsv);
   phext_chain_list_free(&chains);
   printf("%s %s (%d ops)\n", failures == failures_before ? "PASS" : "FAIL", case_name, ops);
   return ops;
}

int main(int argc, char** argv)
{
   const char* root = argc > 1 ? argv[1] : "tests/fixtures/ranges";
   char path[1024];
   FILE* index;
   char line[8192];
   int cases = 0;
   int cases_passed = 0;
   int total_ops = 0;

   snprintf(path, sizeof(path), "%s/cases.tsv", root);
   index = fopen(path, "rb");
   if (index == 0)
   {
      printf("test_range_fixtures: cannot open %s\n", path);
      return 1;
   }
   while (fgets(line, sizeof(line), index) != 0)
   {
      char* fields[4];
      int count = split_tabs(line, fields, 4);
      int failures_before;
      if (count < 1 || strcmp(fields[0], "case") == 0 || fields[0][0] == 0) { continue; }
      failures_before = failures;
      total_ops += run_case(root, fields[0]);
      cases += 1;
      if (failures == failures_before) { cases_passed += 1; }
   }
   fclose(index);
   printf("test_range_fixtures: %d/%d cases passed, %d ops, %d checks, %d failures\n",
          cases_passed, cases, total_ops, checks, failures);
   return failures == 0 ? 0 : 1;
}
