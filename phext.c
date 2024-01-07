#include "phext.h"
#include <stdio.h>

/* phext - internal module functions */

/* ------------------------------------------------------------------------ */
int advance_coordinate(int* spot)
{
   int test = *spot;
   if (test < PHEXT_MAXIMUM_INDEX)
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
   char buffer[8];
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
         memset(buffer, 0, 8);
         i = 0;
         ++state;
         ++a;
         continue;
      }      
      buffer[i] = next;
      ++i;
      if (i > 7)
      {
         break;
      }
      ++a;
   }
   buffer[i] = 0;
   value = atoi(buffer);
   set_coordinate_by_state(state, coord, value);
   memset(buffer, 0, 8);
}

/* ------------------------------------------------------------------------ */
char *phext_get_address(phext_coordinate coord)
{
   char* buffer = (char*) malloc(PHEXT_BUFFER_SIZE);
   sprintf(buffer, "%d.%d.%d/%d.%d.%d/%d.%d.%d\0",
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