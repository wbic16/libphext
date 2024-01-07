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

  #define BUFFER_LIMIT 1000000
  char buffer[BUFFER_LIMIT];
  char* filename = "sample.phext";
  if (argc >= 3)
  {
    filename = argv[2];
  }
  FILE* fp = fopen(filename, "r");
  if (fp)
  {
    int bytes = fread(buffer, sizeof(char), BUFFER_LIMIT, fp);
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