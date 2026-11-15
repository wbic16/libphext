/*
 * seal_tool: cross-implementation seal checker (test support, not installed).
 *   seal_tool seal   <in> <out>   — seal a file with the C port
 *   seal_tool unseal <in> <out>   — unseal a file with the C port
 * Used to verify byte compatibility with libphext-rs in both directions.
 */
#include "phext_seal.h"

#include <stdio.h>

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

static int write_file(const char* path, const unsigned char* data, size_t length)
{
   FILE* fp = fopen(path, "wb");
   if (!fp) { return 0; }
   if (length > 0 && fwrite(data, 1, length, fp) != length)
   {
      fclose(fp);
      return 0;
   }
   fclose(fp);
   return 1;
}

int main(int argc, char** argv)
{
   if (argc != 4)
   {
      fprintf(stderr, "usage: seal_tool seal|unseal <in> <out>\n");
      return 2;
   }
   size_t length = 0;
   unsigned char* input = read_file(argv[2], &length);
   if (!input)
   {
      fprintf(stderr, "cannot read %s\n", argv[2]);
      return 1;
   }
   phext_buffer out;
   int rc;
   if (strcmp(argv[1], "seal") == 0)
   {
      rc = phext_seal_bytes(input, length, &out);
   }
   else if (strcmp(argv[1], "unseal") == 0)
   {
      rc = phext_unseal(input, length, &out);
   }
   else
   {
      fprintf(stderr, "unknown mode %s\n", argv[1]);
      free(input);
      return 2;
   }
   free(input);
   if (rc != PHEXT_SEAL_OK)
   {
      fprintf(stderr, "%s failed\n", argv[1]);
      return 1;
   }
   if (!write_file(argv[3], out.data, out.length))
   {
      fprintf(stderr, "cannot write %s\n", argv[3]);
      phext_buffer_free(&out);
      return 1;
   }
   phext_buffer_free(&out);
   return 0;
}
