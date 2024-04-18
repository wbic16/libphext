#include "phext.h"
#include <stdio.h>

int main(int argc, char** argv)
{
  phext_coordinate coord;
  if (argc >= 2)
  {
    phext_set_coordinate(&coord, argv[1]);    
  }
  else
  {
    phext_set_coordinate(&coord, "16.7.5/5.12.1/42.17.16");
  }

  char* address = phext_get_address(coord);    
  
  const int BUFFER_LIMIT = 1024*1024*1024; // 1GB = 500ms on an average SSD
  char* buffer = (char*)malloc(BUFFER_LIMIT);
  char* filename = "sample.phext";
  if (argc >= 3)
  {
    filename = argv[2];
  }
  FILE* fp = fopen(filename, "r");
  if (fp)
  {
    size_t bytes = fread(buffer, sizeof(char), BUFFER_LIMIT, fp);
    if (bytes == BUFFER_LIMIT) {
      printf("WARNING: 1 GB limit reached - input possibly truncated.\n");
    }
    fclose(fp);
  }
  else
  {
    printf("Unable to find %s.\n", filename);
  }

  char* result = phext_fetch_text(buffer, coord);
  if (result)
  {
    if (strlen(result) > 0)
    {
      printf("%s\n", result);
    }
    free(result);
  }
  if (address)
  {
    free(address);
  }
  return 0;
}