#include "phext_seal.h"

#include <lzma.h>
#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------------------ */
int phext_seal_bytes(const unsigned char* input, size_t length, phext_buffer* out)
{
   if (out == 0) { return PHEXT_SEAL_ERROR; }
   out->data = 0;
   out->length = 0;
   if (input == 0) { input = (const unsigned char*) ""; length = 0; }

   lzma_stream stream = LZMA_STREAM_INIT;
   lzma_ret ret = lzma_easy_encoder(&stream, PHEXT_SEAL_PRESET, LZMA_CHECK_CRC64);
   if (ret != LZMA_OK)
   {
      lzma_end(&stream);
      return PHEXT_SEAL_ERROR;
   }

   /* grow the output in chunks; xz of empty input is a small fixed stream */
   size_t capacity = length + 1024;
   unsigned char* buffer = (unsigned char*) malloc(capacity);
   if (buffer == 0)
   {
      lzma_end(&stream);
      return PHEXT_SEAL_ERROR;
   }

   stream.next_in = input;
   stream.avail_in = length;
   stream.next_out = buffer;
   stream.avail_out = capacity;

   for (;;)
   {
      ret = lzma_code(&stream, LZMA_FINISH);
      if (ret == LZMA_STREAM_END) { break; }
      if (ret != LZMA_OK)
      {
         free(buffer);
         lzma_end(&stream);
         return PHEXT_SEAL_ERROR;
      }
      if (stream.avail_out == 0)
      {
         size_t used = capacity - stream.avail_out;
         capacity *= 2;
         unsigned char* next = (unsigned char*) realloc(buffer, capacity);
         if (next == 0)
         {
            free(buffer);
            lzma_end(&stream);
            return PHEXT_SEAL_ERROR;
         }
         buffer = next;
         stream.next_out = buffer + used;
         stream.avail_out = capacity - used;
      }
   }

   out->length = capacity - stream.avail_out;
   out->data = (unsigned char*) realloc(buffer, out->length + 1);
   out->data[out->length] = 0;
   lzma_end(&stream);
   return PHEXT_SEAL_OK;
}

/* ------------------------------------------------------------------------ */
int phext_seal(const char* input, phext_buffer* out)
{
   if (input == 0) { input = ""; }
   return phext_seal_bytes((const unsigned char*) input, strlen(input), out);
}

/* ------------------------------------------------------------------------ */
int phext_unseal(const unsigned char* data, size_t length, phext_buffer* out)
{
   if (out == 0) { return PHEXT_SEAL_ERROR; }
   out->data = 0;
   out->length = 0;
   if (data == 0 || length == 0) { return PHEXT_SEAL_ERROR; }

   lzma_stream stream = LZMA_STREAM_INIT;
   lzma_ret ret = lzma_stream_decoder(&stream, UINT64_MAX, 0);
   if (ret != LZMA_OK)
   {
      lzma_end(&stream);
      return PHEXT_SEAL_ERROR;
   }

   size_t capacity = length * 4 + 4096;
   unsigned char* buffer = (unsigned char*) malloc(capacity);
   if (buffer == 0)
   {
      lzma_end(&stream);
      return PHEXT_SEAL_ERROR;
   }

   stream.next_in = data;
   stream.avail_in = length;
   stream.next_out = buffer;
   stream.avail_out = capacity;

   int ended = 0;
   while (!ended)
   {
      ret = lzma_code(&stream, LZMA_RUN);
      if (ret == LZMA_STREAM_END)
      {
         ended = 1;
         break;
      }
      if (ret != LZMA_OK)
      {
         free(buffer);
         lzma_end(&stream);
         return PHEXT_SEAL_ERROR;
      }
      if (stream.avail_out == 0)
      {
         size_t used = capacity - stream.avail_out;
         capacity *= 2;
         unsigned char* next = (unsigned char*) realloc(buffer, capacity);
         if (next == 0)
         {
            free(buffer);
            lzma_end(&stream);
            return PHEXT_SEAL_ERROR;
         }
         buffer = next;
         stream.next_out = buffer + used;
         stream.avail_out = capacity - used;
      }
      if (stream.avail_in == 0 && stream.avail_out != 0)
      {
         /* input exhausted before the stream ended: not a valid seal */
         free(buffer);
         lzma_end(&stream);
         return PHEXT_SEAL_ERROR;
      }
   }

   out->length = capacity - stream.avail_out;
   out->data = (unsigned char*) realloc(buffer, out->length + 1);
   out->data[out->length] = 0;
   lzma_end(&stream);
   return PHEXT_SEAL_OK;
}

/* ------------------------------------------------------------------------ */
int phext_unseal_string(const unsigned char* data, size_t length, char** out)
{
   if (out == 0) { return PHEXT_SEAL_ERROR; }
   *out = 0;
   phext_buffer bytes;
   if (phext_unseal(data, length, &bytes) != PHEXT_SEAL_OK) { return PHEXT_SEAL_ERROR; }
   if (!phext_is_valid_utf8(bytes.data, bytes.length))
   {
      phext_buffer_free(&bytes);
      return PHEXT_SEAL_ERROR;
   }
   *out = (char*) bytes.data; /* phext_buffer is already NUL-terminated */
   return PHEXT_SEAL_OK;
}
