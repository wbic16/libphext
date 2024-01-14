#include <stdio.h>
#include <string.h>
#include "phext.h"

phext_coordinate phext_sql_tablespec_coordinate()
{
    phext_coordinate result;
    result.Z.library = 1;
    result.Z.shelf = 1;
    result.Z.series = 1;
    result.Y.collection = 1;
    result.Y.volume = 1;
    result.Y.book = 1;
    result.X.chapter = 1;
    result.X.section = 1;
    result.X.scroll = 1;

    return result;
}

// -----------------------------------------------------------------------------------------------------------
// @fn phext_sql_select
// -----------------------------------------------------------------------------------------------------------
void phext_sql_select(const char* filename, const char* table, const char* fields, const char* limit)
{
  FILE* fp = fopen(filename, "r");
  if (fp)
  {
    fseek(fp, 0, SEEK_END);
    int bytes = ftell(fp);
    rewind(fp);
    char* phext = malloc(bytes);
    fread(phext, 1, bytes, fp);
    fclose(fp);

    phext_coordinate tablespec = phext_sql_tablespec_coordinate();
    char* phext_address = phext_get_address(tablespec);

    char* spec = phext_fetch_text(phext, tablespec);
    char* token = spec;
    int table_index = 1;
    int scrolls = 0;
    token = strtok(spec, "\n");
    if (strcmp("tables", token) != 0)
    {
        fprintf(stderr, "Unable to locate phext sql tables at %s (expected 'tables', but found '%s')\n", phext_address, token);
        return;
    }
    int found = 0;
    while (token != 0)
    {
      ++table_index;
      token = strtok(0, "\n");
      if (token != 0 && strlen(token) > 0)
       {
        if (strncmp(table, token, strlen(table)) == 0)
        {
          found = 1;
          token = strtok(token, ",");
          token = strtok(0, "\n");
          scrolls = atoi(token);
          // fprintf(stderr, "Found table = '%s', checked scroll limit: %d\n", table, scrolls);
          break;
        }
      }
    }

    if (found == 1)
    {
      phext_coordinate data_address = tablespec;
      data_address.X.section = table_index;
            
      char* temp = malloc(strlen(fields) + 1);
      strcpy(temp, fields);
      token = strtok(temp, ",");
      char* field1 = 0;
      char* field2 = 0;
      char* field3 = 0;
      int field1length = 0;
      int field2length = 0;
      int field3length = 0;
      int row = 0;
      int testlength;
      if (token != 0)
      {
        field1length = strlen(token);
        field1 = malloc(field1length + 1);
        strcpy(field1, token);
      }

      token = strtok(0, ",");
      if (token != 0)
      {
        field2length = strlen(token);
        field2 = malloc(field2length + 1);
        strcpy(field2, token);

        token = strtok(0, ",");
        if (token != 0)
        {
          field3length = strlen(token);
          field3 = malloc(field3length + 1);
          strcpy(field3, token);
        }
      }
      free(temp);
            
      for (int i = 1; i <= scrolls; ++i)
      {        
        data_address.X.scroll = i;
        char* data = phext_fetch_text(phext, data_address);

        token = strtok(data, "\n");
        if (token != 0)
        {
          testlength = strlen(token);
          if (field1length == testlength && strncmp(field1, token, testlength) == 0)
          {
            while (token != 0)
            {
              token = strtok(0, "\n");
              if (token != 0)
              {
                if (limit == 0 || strcmp(limit, token) == 0)
                {
                  fprintf(stderr, "Row %d: %s\n", row, token);
                  ++row;
                }
              }
            }
          }
          if (field2length == testlength && strncmp(field2, token, testlength) == 0)
          {
            while (token != 0)
            {
              token = strtok(0, "\n");
              if (token != 0)
              {
                if (limit == 0 || strcmp(limit, token) == 0)
                {
                  fprintf(stderr, "Row %d: %s\n", row, token);
                  ++row;
                }
              }
            }
          }
          if (field3length == testlength && strncmp(field3, token, testlength) == 0)
          {
            while (token != 0)
            {
              token = strtok(0, "\n");
              if (token != 0)
              {
                if (limit == 0 || strcmp(limit, token) == 0)
                {
                  fprintf(stderr, "Row %d: %s\n", row, token);
                  ++row;
                }
              }
            }
          }
        }
        free(data);
      }
    }
    else
    {
      fprintf(stderr, "unable to locate table '%s'\n", table);
    }
    
    free(spec);
    free(phext_address);
  }
}

// -----------------------------------------------------------------------------------------------------------
// @fn phext_sql_insert
// -----------------------------------------------------------------------------------------------------------
void phext_sql_insert(const char* filename, int argc, char** argv)
{    
}

// -----------------------------------------------------------------------------------------------------------
// @fn phext_sql_update
// -----------------------------------------------------------------------------------------------------------
void phext_sql_update(const char* filename, int argc, char** argv)
{    
}

// -----------------------------------------------------------------------------------------------------------
// @fn phext_sql_delete
// -----------------------------------------------------------------------------------------------------------
void phext_sql_delete(const char* filename, int argc, char** argv)
{    
}


int main(int argc, char** argv)
{
  // printf("phesql v1.0\n");

  if (argc < 3)
  {
    fprintf(stderr, "Usage: phesql <file> <command> <options>\n");
    return 1;
  }
  const char* file = argv[1];
  const char* command = argv[2];

  if (strcmp("select", command) == 0)
  {
    if (argc < 5)
    {
      fprintf(stderr, "Usage: phesql select <file> <table> <field_list>\n");
      return 1;
    }
    const char* table = argv[3];
    const char* fields = argv[4];    
    const char* limit = argc >= 6 ? argv[5] : 0;
    phext_sql_select(file, table, fields, limit);
    return 0;
  }

  if (strcmp("insert", command) == 0)
  {
    phext_sql_insert(file, argc, argv);
    return 0;
  }

  if (strcmp("update", command) == 0)
  {
    phext_sql_update(file, argc, argv);
    return 0;
  }

  if (strcmp("delete", command) == 0)
  {
    phext_sql_delete(file, argc, argv);
    return 0;
  }

  fprintf(stderr, "Unknown command: %s\nNote: phesql is case-sensitive.\n", command);
  return 1;
}