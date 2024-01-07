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

#define PHEXT_LIBRARY_BREAK          '\x01' /* 11th dimension: historic: start of heading   */
#define HISTORIC_START_OF_TEXT       '\x02' /* start of text - unchanged                    */
#define HISTORIC_END_OF_TEXT         '\x03' /*     ETX - backwards-compatible               */
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
#define PHEXT_BUFFER_SIZE            64     /* large enough for truly huge phext addresses  */

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
#define PHEXT_MAXIMUM_INDEX 999 /* you really don't need more than this with 9 dimensions */
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
