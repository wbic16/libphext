libphext
--------
libphext is a C library that provides a reference parser for fetching text nodes from phext buffers.

Files
-----
* main.c: a reference parser named 'phextio' after phext.io
* Makefile: gcc toolchain compatible
  * `make test`: Run regression tests against sample.phext
  * `make`: compiles phext.o and links phextio
* phext.h: Header definitions
  * `phext_coordinate`: encodes the standard Library.Shelf.Series/Collection.Volume.Book/Chapter.Section.Scroll Coordinate
  * `phext_fetch_text`: produces the contents of the text file at the given coordinates within the phext buffer - allocates a new null-terminated buffer that you need to manage
  * `phext_initialize_coordinate`: Sets your coordinates to 1.1.1/1.1.1/1.1.1
  * `phext_set_coordinate`: Sets your coordinates to match a given string representation
  * `phext_get_address`: translates a phext coordinate back to dotted-triplet-triplet form
  * `phext_validate_coordinate`: used to confirm whether or not a coordinate is realistic
  * `phext_scroll_break`: manages your coordinate for a SCROLL_BREAK
  * `phext_section_break`: manages your coordinate for a SECTION_BREAK
  * `phext_chapter_break`: manages your coordinate for a CHAPTER_BREAK
  * `phext_book_break`: manages your coordinate for a BOOK_BREAK  
  * `phext_volume_break`: manages your coordinate for a VOLUME_BREAK
  * `phext_collection_break`: manages your coordinate for a COLLECTION_BREAK
  * `phext_series_break`: manages your coordinate for a SERIES_BREAK
  * `phext_shelf_break`: manages your coordinate for a SHELF_BREAK
  * `phext_library_break`: manages your coordinate for a LIBRARY_BREAK
* phext.c: Reference Parser Implementation