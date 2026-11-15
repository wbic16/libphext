#include "phext.h"
#include <stdio.h>
#include <limits.h>

/* phext - internal module functions */

/* ------------------------------------------------------------------------ */
int advance_coordinate(int* spot)
{
   /* 0.4.0: no artificial cap — components advance to the native int width.
      The only failure point is INT_MAX, where incrementing would overflow. */
   int test = *spot;
   if (test < INT_MAX)
   {
      *spot = test + 1;
      return 1;
   }

   return 0;
}

void check_region_end(char* p, char** end, int* stage)
{
   char next = *p;
   if (*stage == 1)
   {
      *end = p - 1;
      *stage = 2;
   }
}

/* phext public functions */

/* ------------------------------------------------------------------------ */
char* phext_fetch_text(char *phext, phext_coordinate coord)
{
   phext_coordinate walker;
   phext_initialize_coordinate(&walker);

   char* p = phext;
   char* start = 0;
   char* end = 0;
   int stage = 0;

   while (*p != 0)
   {
      char next = *p;
      if (next == PHEXT_SCROLL_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_scroll_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_SECTION_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_section_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_CHAPTER_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_chapter_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_BOOK_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_book_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_VOLUME_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_volume_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_COLLECTION_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_collection_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_SERIES_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_series_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_SHELF_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_shelf_break(&walker);
         ++p;
         continue;
      }
      if (next == PHEXT_LIBRARY_BREAK)
      {
         check_region_end(p, &end, &stage);
         phext_library_break(&walker);
         ++p;
         continue;
      }

      if (stage == 0 &&
          coord.Z.library == walker.Z.library &&
          coord.Z.shelf == walker.Z.shelf &&
          coord.Z.series == walker.Z.series &&
          coord.Y.collection == walker.Y.collection &&
          coord.Y.volume == walker.Y.volume &&
          coord.Y.book == walker.Y.book &&
          coord.X.chapter == walker.X.chapter &&
          coord.X.section == walker.X.section &&
          coord.X.scroll == walker.X.scroll)
      {
         start = p;
         stage = 1;
      }

      ++p;
      if (stage == 1 && *p == 0)
      {
         end = p - 1;
         stage = 2;
      }
   }

   int length = end - start + 1;
   int buffer_length = length + 1;

   if (start != 0 && end != 0 && length > 0)
   {
      if (length > 0)
      {
         char* buffer = (char*) malloc(buffer_length);
         memcpy(buffer, start, length);
         buffer[length] = 0;
         return buffer;
      }
   }
   
   return 0;
}

/* ------------------------------------------------------------------------ */
void phext_initialize_coordinate(phext_coordinate *coord)
{
   if (coord == 0)
   {
      return;
   }

   coord->Z.library = 1;
   coord->Z.shelf = 1;
   coord->Z.series = 1;
   coord->Y.collection = 1;
   coord->Y.volume = 1;
   coord->Y.book = 1;
   coord->X.chapter = 1;
   coord->X.section = 1;
   coord->X.scroll = 1;
}

/* ------------------------------------------------------------------------ */
void set_coordinate_by_state(int state, phext_coordinate* coord, int value)
{
   switch (state)
   {
      case 8:
         coord->X.scroll = value;         
         break;
      case 7:
         coord->X.section = value;
         break;
      case 6:
         coord->X.chapter = value;
         break;
      case 5:
         coord->Y.book = value;
         break;
      case 4:
         coord->Y.volume = value;
         break;
      case 3:
         coord->Y.collection = value;
         break;
      case 2:
         coord->Z.series = value;
         break;
      case 1:
         coord->Z.shelf = value;
         break;
      case 0:
      default:
         coord->Z.library = value;
         break;
   }
}

/* ------------------------------------------------------------------------ */
void phext_set_coordinate(phext_coordinate *coord, char *address)
{
   char* a = address;
   int i = 0;
   int state = 0;
   char buffer[16]; /* widened from 8 in 0.4.0: a full int-width component has up to 10 digits */
   char next;
   int value;

   while (*a != 0)
   {
      next = *a;
      if (next == '.' || next == '/')
      {
         buffer[i] = 0;
         value = atoi(buffer);
         set_coordinate_by_state(state, coord, value);
         memset(buffer, 0, 16);
         i = 0;
         ++state;
         ++a;
         continue;
      }      
      buffer[i] = next;
      ++i;
      if (i > 14)
      {
         break;
      }
      ++a;
   }
   buffer[i] = 0;
   value = atoi(buffer);
   set_coordinate_by_state(state, coord, value);
   memset(buffer, 0, 16);
}

/* ------------------------------------------------------------------------ */
char *phext_get_address(phext_coordinate coord)
{
   char* buffer = (char*) malloc(PHEXT_BUFFER_SIZE);
   sprintf(buffer, "%d.%d.%d/%d.%d.%d/%d.%d.%d",
      coord.Z.library, coord.Z.shelf, coord.Z.series,
      coord.Y.collection, coord.Y.volume, coord.Y.book,
      coord.X.chapter, coord.X.section, coord.X.scroll);
   return buffer;      
}

/* ------------------------------------------------------------------------ */
int phext_validate_coordinate(phext_coordinate* coord)
{
   if (coord == 0)
   {
      return PHEXT_INVALID_COORDINATE;
   }

   if (coord->Z.library < PHEXT_MINIMUM_INDEX ||
       coord->Z.shelf < PHEXT_MINIMUM_INDEX ||
       coord->Z.series < PHEXT_MINIMUM_INDEX ||
       coord->Y.collection < PHEXT_MINIMUM_INDEX ||
       coord->Y.volume < PHEXT_MINIMUM_INDEX ||
       coord->Y.book < PHEXT_MINIMUM_INDEX ||
       coord->X.chapter < PHEXT_MINIMUM_INDEX ||
       coord->X.section < PHEXT_MINIMUM_INDEX ||
       coord->X.scroll < PHEXT_MINIMUM_INDEX)
   {
      return PHEXT_INVALID_COORDINATE;
   }

   return PHEXT_NO_ERROR;
}

/* ------------------------------------------------------------------------ */
int phext_scroll_break(phext_coordinate *coord)
{
   if (coord == 0)
   {
      return 0;
   }

   if (advance_coordinate(&(coord->X.scroll)))
   {
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_section_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->X.section)))
   {
      coord->X.scroll = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_chapter_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->X.chapter)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_book_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->Y.book)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      coord->X.chapter = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_volume_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->Y.volume)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      coord->X.chapter = 1;
      coord->Y.book = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_collection_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->Y.collection)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      coord->X.chapter = 1;
      coord->Y.book = 1;
      coord->Y.volume = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_series_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->Z.series)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      coord->X.chapter = 1;
      coord->Y.book = 1;
      coord->Y.volume = 1;
      coord->Y.collection = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_shelf_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->Z.shelf)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      coord->X.chapter = 1;
      coord->Y.book = 1;
      coord->Y.volume = 1;
      coord->Y.collection = 1;
      coord->Z.series = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}

/* ------------------------------------------------------------------------ */
int phext_library_break(phext_coordinate *coord)
{   
   if (advance_coordinate(&(coord->Z.library)))
   {
      coord->X.scroll = 1;
      coord->X.section = 1;
      coord->X.chapter = 1;
      coord->Y.book = 1;
      coord->Y.volume = 1;
      coord->Y.collection = 1;
      coord->Z.series = 1;
      coord->Z.shelf = 1;
      return PHEXT_SUCCESS;
   }

   return PHEXT_FAILURE;
}
/* ========================================================================
 * 0.6.0 implementation — parity with libphext-rs 0.6.0
 * Canonical binary scrolls (rs src/binary.rs), chains + multiverse
 * addressing (rs src/phext.rs chain layer). The xz seal lives in
 * phext_seal.c (the one liblzma-dependent unit).
 * ======================================================================== */

/* ------------------------------------------------------------------------ */
/* owned buffers                                                            */
/* ------------------------------------------------------------------------ */
void phext_buffer_free(phext_buffer* buffer)
{
   if (buffer == 0) { return; }
   free(buffer->data);
   buffer->data = 0;
   buffer->length = 0;
}

static phext_buffer buffer_copy(const unsigned char* data, size_t length)
{
   phext_buffer result;
   result.data = (unsigned char*) malloc(length + 1);
   result.length = length;
   if (length > 0 && data != 0) { memcpy(result.data, data, length); }
   result.data[length] = 0;
   return result;
}

static int buffer_append(phext_buffer* buffer, const unsigned char* data, size_t length)
{
   unsigned char* next = (unsigned char*) realloc(buffer->data, buffer->length + length + 1);
   if (next == 0) { return 0; }
   buffer->data = next;
   if (length > 0) { memcpy(buffer->data + buffer->length, data, length); }
   buffer->length += length;
   buffer->data[buffer->length] = 0;
   return 1;
}

static int buffer_push_byte(phext_buffer* buffer, unsigned char byte)
{
   return buffer_append(buffer, &byte, 1);
}

/* ------------------------------------------------------------------------ */
/* coordinate helpers (reference ordering: library, shelf, series,          */
/* collection, volume, book, chapter, section, scroll)                      */
/* ------------------------------------------------------------------------ */
static int coordinate_compare(phext_coordinate a, phext_coordinate b)
{
   if (a.Z.library != b.Z.library)       { return a.Z.library < b.Z.library ? -1 : 1; }
   if (a.Z.shelf != b.Z.shelf)           { return a.Z.shelf < b.Z.shelf ? -1 : 1; }
   if (a.Z.series != b.Z.series)         { return a.Z.series < b.Z.series ? -1 : 1; }
   if (a.Y.collection != b.Y.collection) { return a.Y.collection < b.Y.collection ? -1 : 1; }
   if (a.Y.volume != b.Y.volume)         { return a.Y.volume < b.Y.volume ? -1 : 1; }
   if (a.Y.book != b.Y.book)             { return a.Y.book < b.Y.book ? -1 : 1; }
   if (a.X.chapter != b.X.chapter)       { return a.X.chapter < b.X.chapter ? -1 : 1; }
   if (a.X.section != b.X.section)       { return a.X.section < b.X.section ? -1 : 1; }
   if (a.X.scroll != b.X.scroll)         { return a.X.scroll < b.X.scroll ? -1 : 1; }
   return 0;
}

static int coordinate_equals(phext_coordinate a, phext_coordinate b)
{
   return coordinate_compare(a, b) == 0;
}

phext_coordinate phext_to_coordinate(const char* address)
{
   phext_coordinate result;
   phext_initialize_coordinate(&result);
   if (address != 0)
   {
      phext_set_coordinate(&result, (char*) address);
   }
   return result;
}

static int is_dimension_break(unsigned char byte)
{
   return byte == (unsigned char) PHEXT_SCROLL_BREAK ||
          byte == (unsigned char) PHEXT_SECTION_BREAK ||
          byte == (unsigned char) PHEXT_CHAPTER_BREAK ||
          byte == (unsigned char) PHEXT_BOOK_BREAK ||
          byte == (unsigned char) PHEXT_VOLUME_BREAK ||
          byte == (unsigned char) PHEXT_COLLECTION_BREAK ||
          byte == (unsigned char) PHEXT_SERIES_BREAK ||
          byte == (unsigned char) PHEXT_SHELF_BREAK ||
          byte == (unsigned char) PHEXT_LIBRARY_BREAK;
}

static void apply_break(phext_coordinate* coord, unsigned char byte)
{
   if (byte == (unsigned char) PHEXT_SCROLL_BREAK)     { phext_scroll_break(coord); }
   if (byte == (unsigned char) PHEXT_SECTION_BREAK)    { phext_section_break(coord); }
   if (byte == (unsigned char) PHEXT_CHAPTER_BREAK)    { phext_chapter_break(coord); }
   if (byte == (unsigned char) PHEXT_BOOK_BREAK)       { phext_book_break(coord); }
   if (byte == (unsigned char) PHEXT_VOLUME_BREAK)     { phext_volume_break(coord); }
   if (byte == (unsigned char) PHEXT_COLLECTION_BREAK) { phext_collection_break(coord); }
   if (byte == (unsigned char) PHEXT_SERIES_BREAK)     { phext_series_break(coord); }
   if (byte == (unsigned char) PHEXT_SHELF_BREAK)      { phext_shelf_break(coord); }
   if (byte == (unsigned char) PHEXT_LIBRARY_BREAK)    { phext_library_break(coord); }
}

/* ------------------------------------------------------------------------ */
/* get_subspace_coordinates — a direct port of the reference algorithm:     */
/* (start, end, best) for the target scroll, or the insertion point + the   */
/* coordinate in effect there when the document does not reach the target.  */
/* ------------------------------------------------------------------------ */
typedef struct
{
   size_t start;
   size_t end;
   phext_coordinate best;
} phext_subspace;

static phext_subspace get_subspace_coordinates(const unsigned char* subspace, size_t max, phext_coordinate target)
{
   phext_coordinate walker;
   phext_coordinate best;
   phext_initialize_coordinate(&walker);
   phext_initialize_coordinate(&best);
   int have_found = 0;
   size_t found = 0;
   size_t position = 0;
   int moved = 1;
   phext_subspace result;

   for (;;)
   {
      if (moved && !have_found)
      {
         if (coordinate_equals(walker, target))
         {
            have_found = 1;
            found = position;
            best = walker;
         }
         else if (coordinate_compare(walker, target) < 0)
         {
            best = walker;
         }
         else
         {
            result.start = position; result.end = position; result.best = best;
            return result;
         }
         moved = 0;
      }
      if (position == max) { break; }

      unsigned char next = subspace[position];
      if (is_dimension_break(next))
      {
         apply_break(&walker, next);
         moved = 1;
         if (coordinate_compare(walker, target) > 0)
         {
            if (have_found) { result.start = found; result.end = position; }
            else            { result.start = position; result.end = position; }
            result.best = best;
            return result;
         }
         if (have_found && found == position) { found = position + 1; }
      }
      position += 1;
   }

   if (have_found) { result.start = found; result.end = max; }
   else            { result.start = max; result.end = max; }
   result.best = best;
   return result;
}

/* breaks from `from` up to `location`, in reference order (highest first) */
static void build_fixup(phext_coordinate from, phext_coordinate location, phext_buffer* fixup)
{
   phext_coordinate cursor = from;
   while (cursor.Z.library < location.Z.library)       { buffer_push_byte(fixup, (unsigned char) PHEXT_LIBRARY_BREAK);    phext_library_break(&cursor); }
   while (cursor.Z.shelf < location.Z.shelf)           { buffer_push_byte(fixup, (unsigned char) PHEXT_SHELF_BREAK);      phext_shelf_break(&cursor); }
   while (cursor.Z.series < location.Z.series)         { buffer_push_byte(fixup, (unsigned char) PHEXT_SERIES_BREAK);     phext_series_break(&cursor); }
   while (cursor.Y.collection < location.Y.collection) { buffer_push_byte(fixup, (unsigned char) PHEXT_COLLECTION_BREAK); phext_collection_break(&cursor); }
   while (cursor.Y.volume < location.Y.volume)         { buffer_push_byte(fixup, (unsigned char) PHEXT_VOLUME_BREAK);     phext_volume_break(&cursor); }
   while (cursor.Y.book < location.Y.book)             { buffer_push_byte(fixup, (unsigned char) PHEXT_BOOK_BREAK);       phext_book_break(&cursor); }
   while (cursor.X.chapter < location.X.chapter)       { buffer_push_byte(fixup, (unsigned char) PHEXT_CHAPTER_BREAK);    phext_chapter_break(&cursor); }
   while (cursor.X.section < location.X.section)       { buffer_push_byte(fixup, (unsigned char) PHEXT_SECTION_BREAK);    phext_section_break(&cursor); }
   while (cursor.X.scroll < location.X.scroll)         { buffer_push_byte(fixup, (unsigned char) PHEXT_SCROLL_BREAK);     phext_scroll_break(&cursor); }
}

/* ------------------------------------------------------------------------ */
/* fetch / insert / replace (reference semantics)                           */
/* ------------------------------------------------------------------------ */
int phext_fetch_buffer(const unsigned char* phext, size_t phext_length, phext_coordinate coord, phext_buffer* out)
{
   if (out == 0) { return PHEXT_FAILURE; }
   phext_subspace parts = get_subspace_coordinates(phext, phext_length, coord);
   size_t length = parts.end > parts.start ? parts.end - parts.start : 0;
   *out = buffer_copy(phext + parts.start, length);
   return PHEXT_SUCCESS;
}

char* phext_fetch(const char* phext, phext_coordinate coord)
{
   phext_buffer out;
   if (phext == 0) { phext = ""; }
   phext_fetch_buffer((const unsigned char*) phext, strlen(phext), coord, &out);
   return (char*) out.data;
}

static char* replace_len(const char* phext, phext_coordinate location, const unsigned char* scroll, size_t scroll_length)
{
   if (phext == 0) { phext = ""; }
   if (scroll == 0) { scroll = (const unsigned char*) ""; scroll_length = 0; }
   size_t max = strlen(phext);
   phext_subspace parts = get_subspace_coordinates((const unsigned char*) phext, max, location);
   phext_buffer fixup = buffer_copy(0, 0);
   build_fixup(parts.best, location, &fixup);

   phext_buffer result = buffer_copy(0, 0);
   buffer_append(&result, (const unsigned char*) phext, parts.start);
   buffer_append(&result, fixup.data, fixup.length);
   buffer_append(&result, scroll, scroll_length);
   buffer_append(&result, (const unsigned char*) phext + parts.end, max - parts.end);
   phext_buffer_free(&fixup);
   return (char*) result.data;
}

char* phext_replace(const char* phext, phext_coordinate location, const char* scroll)
{
   if (scroll == 0) { scroll = ""; }
   return replace_len(phext, location, (const unsigned char*) scroll, strlen(scroll));
}

static char* insert_len(const char* phext, phext_coordinate location, const unsigned char* scroll, size_t scroll_length)
{
   if (phext == 0) { phext = ""; }
   if (scroll == 0) { scroll = (const unsigned char*) ""; scroll_length = 0; }
   size_t max = strlen(phext);
   phext_subspace parts = get_subspace_coordinates((const unsigned char*) phext, max, location);
   phext_buffer fixup = buffer_copy(0, 0);
   build_fixup(parts.best, location, &fixup);

   phext_buffer result = buffer_copy(0, 0);
   buffer_append(&result, (const unsigned char*) phext, parts.end);
   buffer_append(&result, fixup.data, fixup.length);
   buffer_append(&result, scroll, scroll_length);
   buffer_append(&result, (const unsigned char*) phext + parts.end, max - parts.end);
   phext_buffer_free(&fixup);
   return (char*) result.data;
}

char* phext_insert(const char* phext, phext_coordinate location, const char* scroll)
{
   if (scroll == 0) { scroll = ""; }
   return insert_len(phext, location, (const unsigned char*) scroll, strlen(scroll));
}

/* ------------------------------------------------------------------------ */
/* phokenize (reference algorithm, byte form)                               */
/* ------------------------------------------------------------------------ */
static void phoken_list_push(phext_phoken_list* list, phext_coordinate coord, const unsigned char* data, size_t length)
{
   list->items = (phext_positioned_bytes*) realloc(list->items, (list->count + 1) * sizeof(phext_positioned_bytes));
   list->items[list->count].coord = coord;
   phext_buffer copy = buffer_copy(data, length);
   list->items[list->count].data = copy.data;
   list->items[list->count].length = copy.length;
   list->count += 1;
}

phext_phoken_list phext_phokenize_bytes(const unsigned char* input, size_t max)
{
   phext_phoken_list result;
   result.items = 0;
   result.count = 0;
   if (input == 0) { input = (const unsigned char*) ""; max = 0; }

   phext_coordinate coord;
   phext_initialize_coordinate(&coord);
   size_t pi = 0;
   for (;;)
   {
      phext_coordinate begin = coord;
      int have_text = 0;
      size_t text_start = 0;
      size_t text_end = max;
      while (pi < max)
      {
         unsigned char byte = input[pi];
         if (is_dimension_break(byte))
         {
            apply_break(&coord, byte);
            if (have_text)
            {
               text_end = pi;
               pi += 1;
               break;
            }
         }
         else if (!have_text)
         {
            have_text = 1;
            text_start = pi;
            begin = coord;
         }
         pi += 1;
      }
      if (have_text) { phoken_list_push(&result, begin, input + text_start, text_end - text_start); }
      else           { phoken_list_push(&result, begin, 0, 0); }
      if (pi >= max) { break; }
   }
   return result;
}

phext_phoken_list phext_phokenize(const char* input)
{
   if (input == 0) { input = ""; }
   return phext_phokenize_bytes((const unsigned char*) input, strlen(input));
}

void phext_phoken_list_free(phext_phoken_list* list)
{
   if (list == 0) { return; }
   size_t i;
   for (i = 0; i < list->count; ++i) { free(list->items[i].data); }
   free(list->items);
   list->items = 0;
   list->count = 0;
}

/* ------------------------------------------------------------------------ */
/* UTF-8 validation (strict: no overlongs, no surrogates, <= U+10FFFF)      */
/* ------------------------------------------------------------------------ */
int phext_is_valid_utf8(const unsigned char* data, size_t length)
{
   size_t i = 0;
   while (i < length)
   {
      unsigned char lead = data[i];
      if (lead < 0x80) { i += 1; continue; }
      size_t extra;
      unsigned long codepoint;
      unsigned long minimum;
      if ((lead & 0xE0) == 0xC0)      { extra = 1; codepoint = lead & 0x1F; minimum = 0x80; }
      else if ((lead & 0xF0) == 0xE0) { extra = 2; codepoint = lead & 0x0F; minimum = 0x800; }
      else if ((lead & 0xF8) == 0xF0) { extra = 3; codepoint = lead & 0x07; minimum = 0x10000; }
      else { return 0; }
      if (i + extra >= length) { return 0; }
      size_t j;
      for (j = 1; j <= extra; ++j)
      {
         unsigned char cont = data[i + j];
         if ((cont & 0xC0) != 0x80) { return 0; }
         codepoint = (codepoint << 6) | (cont & 0x3F);
      }
      if (codepoint < minimum) { return 0; }                    /* overlong */
      if (codepoint >= 0xD800 && codepoint <= 0xDFFF) { return 0; } /* surrogate */
      if (codepoint > 0x10FFFF) { return 0; }
      i += extra + 1;
   }
   return 1;
}

/* ------------------------------------------------------------------------ */
/* canonical binary scrolls                                                 */
/* ------------------------------------------------------------------------ */
static const char PHEXT_BASE64_ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define PHEXT_BINARY_PREFIX "phext-binary:1:base64\n"
#define PHEXT_BINARY_PREFIX_LENGTH 22

int phext_is_binary_scroll(const char* scroll)
{
   if (scroll == 0) { return 0; }
   return strncmp(scroll, PHEXT_BINARY_PREFIX, PHEXT_BINARY_PREFIX_LENGTH) == 0;
}

char* phext_encode_scroll(const unsigned char* payload, size_t length)
{
   size_t body_length = ((length + 2) / 3) * 4;
   char* out = (char*) malloc(PHEXT_BINARY_PREFIX_LENGTH + body_length + 1);
   memcpy(out, PHEXT_BINARY_PREFIX, PHEXT_BINARY_PREFIX_LENGTH);
   char* body = out + PHEXT_BINARY_PREFIX_LENGTH;
   size_t i = 0;
   size_t o = 0;
   while (i < length)
   {
      size_t remaining = length - i;
      unsigned int triple = payload[i] << 16;
      if (remaining > 1) { triple |= payload[i + 1] << 8; }
      if (remaining > 2) { triple |= payload[i + 2]; }
      body[o++] = PHEXT_BASE64_ALPHABET[(triple >> 18) & 0x3F];
      body[o++] = PHEXT_BASE64_ALPHABET[(triple >> 12) & 0x3F];
      body[o++] = remaining > 1 ? PHEXT_BASE64_ALPHABET[(triple >> 6) & 0x3F] : '=';
      body[o++] = remaining > 2 ? PHEXT_BASE64_ALPHABET[triple & 0x3F] : '=';
      i += 3;
   }
   body[o] = 0;
   return out;
}

static int base64_value(unsigned char ch)
{
   if (ch >= 'A' && ch <= 'Z') { return ch - 'A'; }
   if (ch >= 'a' && ch <= 'z') { return ch - 'a' + 26; }
   if (ch >= '0' && ch <= '9') { return ch - '0' + 52; }
   if (ch == '+') { return 62; }
   if (ch == '/') { return 63; }
   return -1;
}

int phext_decode_scroll(const char* scroll, phext_buffer* out)
{
   if (scroll == 0 || out == 0) { return 0; }
   if (!phext_is_binary_scroll(scroll)) { return 0; }
   const unsigned char* body = (const unsigned char*) scroll + PHEXT_BINARY_PREFIX_LENGTH;
   size_t body_length = strlen((const char*) body);
   if (body_length % 4 != 0) { return 0; }

   size_t padding = 0;
   if (body_length > 0 && body[body_length - 1] == '=') { padding += 1; }
   if (body_length > 1 && body[body_length - 2] == '=') { padding += 1; }

   phext_buffer result = buffer_copy(0, 0);
   size_t i;
   for (i = 0; i < body_length; i += 4)
   {
      int v[4];
      int k;
      for (k = 0; k < 4; ++k)
      {
         unsigned char ch = body[i + k];
         if (ch == '=')
         {
            if (i + 4 != body_length) { phext_buffer_free(&result); return 0; } /* '=' only in the final group */
            v[k] = 0;
         }
         else
         {
            v[k] = base64_value(ch);
            if (v[k] < 0) { phext_buffer_free(&result); return 0; }
         }
      }
      /* '=' placement: only the last one or two chars of the final group */
      if (body[i + 2] == '=' && body[i + 3] != '=') { phext_buffer_free(&result); return 0; }
      if (body[i + 0] == '=' || body[i + 1] == '=') { phext_buffer_free(&result); return 0; }
      unsigned int triple = (v[0] << 18) | (v[1] << 12) | (v[2] << 6) | v[3];
      int is_last = (i + 4 == body_length);
      /* canonical form: unused trailing bits of a padded group are zero */
      if (is_last && padding == 1 && (triple & 0x03) != 0) { phext_buffer_free(&result); return 0; }
      if (is_last && padding == 2 && (triple & 0x0F) != 0) { phext_buffer_free(&result); return 0; }
      buffer_push_byte(&result, (unsigned char) ((triple >> 16) & 0xFF));
      if (!(is_last && padding == 2)) { buffer_push_byte(&result, (unsigned char) ((triple >> 8) & 0xFF)); }
      if (!(is_last && padding >= 1)) { buffer_push_byte(&result, (unsigned char) (triple & 0xFF)); }
   }
   *out = result;
   return 1;
}

phext_buffer phext_fetch_binary(const char* phext, phext_coordinate target)
{
   char* scroll = phext_fetch(phext, target);
   phext_buffer decoded;
   if (phext_decode_scroll(scroll, &decoded))
   {
      free(scroll);
      return decoded;
   }
   phext_buffer result = buffer_copy((const unsigned char*) scroll, strlen(scroll));
   free(scroll);
   return result;
}

char* phext_insert_binary(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length)
{
   char* encoded = phext_encode_scroll(payload, length);
   char* result = phext_insert(phext, location, encoded);
   free(encoded);
   return result;
}

char* phext_replace_binary(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length)
{
   char* encoded = phext_encode_scroll(payload, length);
   char* result = phext_replace(phext, location, encoded);
   free(encoded);
   return result;
}

/* the stored form of an incoming scroll update, per the ingestion rule */
static phext_buffer stored_form(const unsigned char* payload, size_t length)
{
   if (phext_is_valid_utf8(payload, length))
   {
      return buffer_copy(payload, length);
   }
   char* encoded = phext_encode_scroll(payload, length);
   phext_buffer result = buffer_copy((const unsigned char*) encoded, strlen(encoded));
   free(encoded);
   return result;
}

char* phext_insert_bytes(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length)
{
   phext_buffer stored = stored_form(payload, length);
   char* result = insert_len(phext, location, stored.data, stored.length);
   phext_buffer_free(&stored);
   return result;
}

char* phext_replace_bytes(const char* phext, phext_coordinate location, const unsigned char* payload, size_t length)
{
   phext_buffer stored = stored_form(payload, length);
   char* result = replace_len(phext, location, stored.data, stored.length);
   phext_buffer_free(&stored);
   return result;
}

/* ------------------------------------------------------------------------ */
/* ingest: phokenize as bytes, dedupe by coordinate (last wins, as the      */
/* reference HashMap insert does), sort, and implode (dephokenize: skip     */
/* empty scrolls; append each non-empty scroll with synthesized breaks).    */
/* ------------------------------------------------------------------------ */
static void append_scroll_text(phext_buffer* out, phext_coordinate* cursor, phext_coordinate target, const unsigned char* text, size_t length)
{
   phext_buffer breaks = buffer_copy(0, 0);
   phext_coordinate walker = *cursor;
   while (coordinate_compare(walker, target) < 0)
   {
      if (walker.Z.library < target.Z.library)            { buffer_push_byte(&breaks, (unsigned char) PHEXT_LIBRARY_BREAK);    phext_library_break(&walker);    continue; }
      if (walker.Z.shelf < target.Z.shelf)                { buffer_push_byte(&breaks, (unsigned char) PHEXT_SHELF_BREAK);      phext_shelf_break(&walker);      continue; }
      if (walker.Z.series < target.Z.series)              { buffer_push_byte(&breaks, (unsigned char) PHEXT_SERIES_BREAK);     phext_series_break(&walker);     continue; }
      if (walker.Y.collection < target.Y.collection)      { buffer_push_byte(&breaks, (unsigned char) PHEXT_COLLECTION_BREAK); phext_collection_break(&walker); continue; }
      if (walker.Y.volume < target.Y.volume)              { buffer_push_byte(&breaks, (unsigned char) PHEXT_VOLUME_BREAK);     phext_volume_break(&walker);     continue; }
      if (walker.Y.book < target.Y.book)                  { buffer_push_byte(&breaks, (unsigned char) PHEXT_BOOK_BREAK);       phext_book_break(&walker);       continue; }
      if (walker.X.chapter < target.X.chapter)            { buffer_push_byte(&breaks, (unsigned char) PHEXT_CHAPTER_BREAK);    phext_chapter_break(&walker);    continue; }
      if (walker.X.section < target.X.section)            { buffer_push_byte(&breaks, (unsigned char) PHEXT_SECTION_BREAK);    phext_section_break(&walker);    continue; }
      if (walker.X.scroll < target.X.scroll)              { buffer_push_byte(&breaks, (unsigned char) PHEXT_SCROLL_BREAK);     phext_scroll_break(&walker);     continue; }
      break; /* unreachable while walker < target */
   }
   buffer_append(out, breaks.data, breaks.length);
   buffer_append(out, text, length);
   phext_buffer_free(&breaks);
}

static int positioned_compare(const void* a, const void* b)
{
   const phext_positioned_bytes* pa = (const phext_positioned_bytes*) a;
   const phext_positioned_bytes* pb = (const phext_positioned_bytes*) b;
   return coordinate_compare(pa->coord, pb->coord);
}

char* phext_ingest(const unsigned char* bytes, size_t length)
{
   phext_phoken_list parts = phext_phokenize_bytes(bytes, length);

   /* dedupe: later entries replace earlier ones at the same coordinate */
   phext_phoken_list unique;
   unique.items = 0;
   unique.count = 0;
   size_t i;
   for (i = 0; i < parts.count; ++i)
   {
      size_t j;
      int replaced = 0;
      for (j = 0; j < unique.count; ++j)
      {
         if (coordinate_equals(unique.items[j].coord, parts.items[i].coord))
         {
            free(unique.items[j].data);
            phext_buffer copy = buffer_copy(parts.items[i].data, parts.items[i].length);
            unique.items[j].data = copy.data;
            unique.items[j].length = copy.length;
            replaced = 1;
            break;
         }
      }
      if (!replaced)
      {
         phoken_list_push(&unique, parts.items[i].coord, parts.items[i].data, parts.items[i].length);
      }
   }
   phext_phoken_list_free(&parts);

   if (unique.count > 1) { qsort(unique.items, unique.count, sizeof(phext_positioned_bytes), positioned_compare); }

   phext_buffer out = buffer_copy(0, 0);
   phext_coordinate cursor;
   phext_initialize_coordinate(&cursor);
   for (i = 0; i < unique.count; ++i)
   {
      phext_positioned_bytes* item = &unique.items[i];
      if (item->length > 0)
      {
         phext_buffer stored = stored_form(item->data, item->length);
         append_scroll_text(&out, &cursor, item->coord, stored.data, stored.length);
         phext_buffer_free(&stored);
      }
      cursor = item->coord;
   }
   phext_phoken_list_free(&unique);
   return (char*) out.data;
}

/* ------------------------------------------------------------------------ */
/* chains + multiverse addressing                                           */
/* ------------------------------------------------------------------------ */
static void chain_list_push_chain(phext_chain_list* list, phext_chain chain)
{
   list->chains = (phext_chain*) realloc(list->chains, (list->count + 1) * sizeof(phext_chain));
   list->chains[list->count] = chain;
   list->count += 1;
}

static void chain_push_member(phext_chain* chain, const unsigned char* data, size_t length)
{
   chain->members = (phext_buffer*) realloc(chain->members, (chain->member_count + 1) * sizeof(phext_buffer));
   chain->members[chain->member_count] = buffer_copy(data, length);
   chain->member_count += 1;
}

/* split a segment on one delimiter byte, preserving empty pieces exactly */
static void split_segment(const unsigned char* data, size_t length, unsigned char delim,
                          const unsigned char*** starts_out, size_t** lengths_out, size_t* count_out)
{
   size_t count = 1;
   size_t i;
   for (i = 0; i < length; ++i) { if (data[i] == delim) { count += 1; } }
   const unsigned char** starts = (const unsigned char**) malloc(count * sizeof(unsigned char*));
   size_t* lengths = (size_t*) malloc(count * sizeof(size_t));
   size_t index = 0;
   size_t start = 0;
   for (i = 0; i <= length; ++i)
   {
      if (i == length || data[i] == delim)
      {
         starts[index] = data + start;
         lengths[index] = i - start;
         index += 1;
         start = i + 1;
      }
   }
   *starts_out = starts;
   *lengths_out = lengths;
   *count_out = count;
}

phext_chain_list phext_parse_chains(const unsigned char* input, size_t length)
{
   phext_chain_list result;
   result.chains = 0;
   result.count = 0;
   if (input == 0) { input = (const unsigned char*) ""; length = 0; }

   int ends_closed = (length > 0 && input[length - 1] == (unsigned char) PHEXT_CHAIN_CLOSE);

   const unsigned char** seg_starts;
   size_t* seg_lengths;
   size_t seg_count;
   split_segment(input, length, (unsigned char) PHEXT_CHAIN_CLOSE, &seg_starts, &seg_lengths, &seg_count);
   if (ends_closed) { seg_count -= 1; } /* the empty tail after a trailing close is the end of the stream, not a new chain */

   size_t i;
   for (i = 0; i < seg_count; ++i)
   {
      phext_chain chain;
      chain.members = 0;
      chain.member_count = 0;
      chain.closed = (ends_closed || i + 1 < seg_count) ? 1 : 0;

      const unsigned char** mem_starts;
      size_t* mem_lengths;
      size_t mem_count;
      split_segment(seg_starts[i], seg_lengths[i], (unsigned char) PHEXT_CHAIN_BREAK, &mem_starts, &mem_lengths, &mem_count);
      size_t j;
      for (j = 0; j < mem_count; ++j)
      {
         chain_push_member(&chain, mem_starts[j], mem_lengths[j]);
      }
      free(mem_starts);
      free(mem_lengths);
      chain_list_push_chain(&result, chain);
   }
   free(seg_starts);
   free(seg_lengths);
   return result;
}

void phext_chain_list_free(phext_chain_list* list)
{
   if (list == 0) { return; }
   size_t i;
   for (i = 0; i < list->count; ++i)
   {
      size_t j;
      for (j = 0; j < list->chains[i].member_count; ++j)
      {
         phext_buffer_free(&list->chains[i].members[j]);
      }
      free(list->chains[i].members);
   }
   free(list->chains);
   list->chains = 0;
   list->count = 0;
}

phext_buffer phext_chain_serialize(const phext_chain* chain)
{
   phext_buffer out = buffer_copy(0, 0);
   size_t i;
   for (i = 0; i < chain->member_count; ++i)
   {
      if (i > 0) { buffer_push_byte(&out, (unsigned char) PHEXT_CHAIN_BREAK); }
      buffer_append(&out, chain->members[i].data, chain->members[i].length);
   }
   if (chain->closed) { buffer_push_byte(&out, (unsigned char) PHEXT_CHAIN_CLOSE); }
   return out;
}

phext_buffer phext_serialize_chains(const phext_chain_list* list)
{
   phext_buffer out = buffer_copy(0, 0);
   size_t i;
   for (i = 0; i < list->count; ++i)
   {
      phext_buffer one = phext_chain_serialize(&list->chains[i]);
      buffer_append(&out, one.data, one.length);
      phext_buffer_free(&one);
   }
   return out;
}

int phext_parse_universe_address(const char* address, size_t* universe_out, phext_coordinate* coord_out)
{
   if (address == 0) { return 0; }
   size_t universe = 1;
   const char* coord_part = address;
   if (address[0] == '$')
   {
      const char* colon = strchr(address, ':');
      if (colon == 0 || colon == address + 1) { return 0; }
      size_t value = 0;
      const char* p;
      for (p = address + 1; p < colon; ++p)
      {
         if (*p < '0' || *p > '9') { return 0; }
         value = value * 10 + (size_t) (*p - '0');
      }
      if (value < 1) { return 0; }
      universe = value;
      coord_part = colon + 1;
   }
   if (universe_out != 0) { *universe_out = universe; }
   if (coord_out != 0) { *coord_out = phext_to_coordinate(coord_part); }
   return 1;
}

int phext_chain_fetch_buffer(const phext_chain* chain, const char* address, phext_buffer* out)
{
   if (out == 0) { return PHEXT_FAILURE; }
   *out = buffer_copy(0, 0);
   if (chain == 0) { return PHEXT_SUCCESS; }
   size_t universe;
   phext_coordinate coord;
   if (!phext_parse_universe_address(address, &universe, &coord)) { return PHEXT_SUCCESS; }
   if (universe < 1 || universe > chain->member_count) { return PHEXT_SUCCESS; }
   phext_buffer member = chain->members[universe - 1];
   phext_buffer_free(out);
   phext_fetch_buffer(member.data, member.length, coord, out);
   return PHEXT_SUCCESS;
}

char* phext_chain_fetch(const phext_chain* chain, const char* address)
{
   phext_buffer out;
   phext_chain_fetch_buffer(chain, address, &out);
   return (char*) out.data;
}
