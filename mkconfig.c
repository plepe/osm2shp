#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

regex_t r_file;
int file_id;
struct shape_file {
  int	id;
  int format;
  char* name;
  int ncolumns;
};
struct shape_file current_shape_file;

void setup_regex() {
  int ret;

  if((ret=regcomp(&r_file, "^file ([a-z]+) \"([^\"]*)\"", REG_EXTENDED))) {
    fprintf(stderr, "Error compiling regular expression: %d\n", ret);
    exit(1);
  }
}

void write_old_file() {
  if(current_shape_file.id==-1)
    return;

  printf("shapefile_new(%d, %i, \"%s\", X", current_shape_file.id, current_shape_file.format, current_shape_file.name);
}

void parse_line(char *row) {
  regmatch_t matches[10];
  char tmp[1024];
  int tmpi;

  if(!(regexec(&r_file, row, 3, matches, 0))) {
    write_old_file();
    current_shape_file.id=++file_id;

    strncpy(tmp, row+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
    tmp[matches[1].rm_eo-matches[1].rm_so]='\0';
    tmpi=
      (strcmp(tmp, "point")?0:
       strcmp(tmp, "line")?1:
       strcmp(tmp, "polygon")?2:-1);
    if(tmpi==-1) {
      fprintf(stderr, "Error parsing type of shapefile, must be point, line or polygon: %s\n", tmp);
      exit(1);
    }
    current_shape_file.format=tmpi;

    strncpy(tmp, row+matches[2].rm_so, matches[2].rm_eo-matches[2].rm_so);
    tmp[matches[2].rm_eo-matches[2].rm_so]='\0';
    current_shape_file.name=malloc(strlen(tmp)+1);
    strcpy(current_shape_file.name, tmp);
  }
}

void main() {
  setup_regex();

  file_id=-1;
  current_shape_file.id=-1;

  FILE *f;
  char row[1024];
  f=fopen("osm2shp.cfg", "r");

  while(fgets(row, 1024, f)) {
    printf("Read: %s", row);

    parse_line(row);
  }

  write_old_file();
}
