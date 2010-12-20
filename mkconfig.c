#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

#define MAX_SHAPEFILES 32
#define MAX_COLUMNS 32
#define MAX_KEYS 32
#define BUFSIZE 1024

regex_t r_file;
regex_t r_column;
regex_t r_key;

struct shape_column {
  int type;
  char *name;
  int nkeys;
  char *keys[MAX_KEYS];
};

struct shape_file {
  int	id;
  int format;
  char* name;
  int ncolumns;
  struct shape_column *columns[MAX_COLUMNS];
};
int file_count;
struct shape_file *all[MAX_SHAPEFILES];
struct shape_file *current_shape_file;
char *SHP_TYPES[]={ "SHPT_POINT", "SHPT_ARC", "SHPT_POLYGON" };
char *COL_TYPES[]={ "FTInteger", "FTString", "FTDouble" };
char *COL_SIZES[]={ "11", "256", "11, 5" };
char *OSM_TYPE[] ={ "node", "way", "polygon" };

void setup_regex() {
  int ret;

  if((ret=regcomp(&r_file, "^file ([a-z]+) \"([^\"]*)\"", REG_EXTENDED))) {
    fprintf(stderr, "Error compiling regular expression: %d\n", ret);
    exit(1);
  }

  if((ret=regcomp(&r_column, "^column ([a-z]+) \"([^\"]*)\"", REG_EXTENDED))) {
    fprintf(stderr, "Error compiling regular expression: %d\n", ret);
    exit(1);
  }

  if((ret=regcomp(&r_key, "^\\w*key \"([^\"]*)\"", REG_EXTENDED))) {
    fprintf(stderr, "Error compiling regular expression: %d\n", ret);
    exit(1);
  }
}

void write_files_setup() {
  int i, col_i;

  for(i=0; i<file_count; i++) {
    current_shape_file=all[i];
    printf("shapefile_new(%d, %s, \"%s\", %i", current_shape_file->id, SHP_TYPES[current_shape_file->format], current_shape_file->name, current_shape_file->ncolumns+1);

    printf(",\n\t\"osm_id\", FTInteger, 11");

    for(col_i=0; col_i<current_shape_file->ncolumns; col_i++) {
      struct shape_column *column=current_shape_file->columns[col_i];
      printf(",\n\t\"%s\", %s, %s", column->name, COL_TYPES[column->type], COL_SIZES[column->type]);
    }

    printf(");\n\n");
  }
}

void write_files_parse(int format) {
  int i, col_i;

  for(i=0; i<file_count; i++) {
    current_shape_file=all[i];

    if(current_shape_file->format!=format)
      continue;

    for(col_i=0; col_i<current_shape_file->ncolumns; col_i++) {
      struct shape_column *column=current_shape_file->columns[col_i];

      if(column->nkeys==0) {
	printf("\tconst char *col%d = g_hash_table_lookup(current_tags, \"%s\");\n", col_i, column->name);
      }
      else {
	int key_i;

	printf("\tconst char *col%d = g_hash_table_lookup(current_tags, \"%s\");\n", col_i, column->keys[0]);
	for(key_i=1; key_i<column->nkeys; key_i++) {
	  printf("\tif (!col%d) col%d = g_hash_table_lookup(current_tags, \"%s\");\n", col_i, col_i, column->keys[key_i]);
	}
      }
    }

    printf("\tshapefile_add_%s(%i, current_id", OSM_TYPE[current_shape_file->format], current_shape_file->id);
    for(col_i=0; col_i<current_shape_file->ncolumns; col_i++) {
      struct shape_column *column=current_shape_file->columns[col_i];
      switch(column->type) {
	case 0:
	  printf(", col%d ? atoi(col%d) : 0", col_i, col_i);
	  break;
	case 1:
	  printf(", col%d", col_i);
	  break;
	case 2:
	  printf(", col%d ? atof(col%d) : 0.0", col_i, col_i);
	  break;
      }
    }
    printf(");\n\n");
  }
}


void parse_line(char *row) {
  regmatch_t matches[10];
  char tmp[BUFSIZE];
  int tmpi;

  if(!(regexec(&r_file, row, 3, matches, 0))) {
    current_shape_file=malloc(sizeof(struct shape_file));
    all[file_count]=current_shape_file;
    current_shape_file->id=file_count++;
    current_shape_file->ncolumns=0;

    strncpy(tmp, row+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
    tmp[matches[1].rm_eo-matches[1].rm_so]='\0';
    tmpi=
      (!strcmp(tmp, "point")?0:
       !strcmp(tmp, "line")?1:
       !strcmp(tmp, "polygon")?2:-1);
    if(tmpi==-1) {
      fprintf(stderr, "Error parsing type of shapefile, must be point, line or polygon: %s\n", tmp);
      exit(1);
    }
    current_shape_file->format=tmpi;

    strncpy(tmp, row+matches[2].rm_so, matches[2].rm_eo-matches[2].rm_so);
    tmp[matches[2].rm_eo-matches[2].rm_so]='\0';
    current_shape_file->name=malloc(strlen(tmp)+1);
    strcpy(current_shape_file->name, tmp);
  }

  if(!(regexec(&r_column, row, 3, matches, 0))) {
    struct shape_column *column=malloc(sizeof(struct shape_file));
    current_shape_file->columns[current_shape_file->ncolumns++]=column;

    strncpy(tmp, row+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
    tmp[matches[1].rm_eo-matches[1].rm_so]='\0';
    tmpi=
      (!strcmp(tmp, "int")?0:
       !strcmp(tmp, "string")?1:
       !strcmp(tmp, "float")?2:-1);
    if(tmpi==-1) {
      fprintf(stderr, "Error parsing type of column, must be int, string or float: %s\n", tmp);
      exit(1);
    }
    column->type=tmpi;

    strncpy(tmp, row+matches[2].rm_so, matches[2].rm_eo-matches[2].rm_so);
    tmp[matches[2].rm_eo-matches[2].rm_so]='\0';
    column->name=malloc(strlen(tmp)+1);
    strcpy(column->name, tmp);

    row=row+matches[0].rm_eo+1;

    column->nkeys=0;
    while(!(regexec(&r_key, row, 2, matches, 0))) {
      strncpy(tmp, row+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
      tmp[matches[1].rm_eo-matches[1].rm_so]='\0';
      column->keys[column->nkeys]=malloc(strlen(tmp)+1);
      strcpy(column->keys[column->nkeys], tmp);
      column->nkeys++;

      row=row+matches[0].rm_eo+1;
    }
  }
}

void main() {
  setup_regex();

  file_count=0;

  FILE *f;
  char row[BUFSIZE];
  f=fopen("osm2shp.cfg", "r");

  while(fgets(row, BUFSIZE, f)) {
    parse_line(row);
  }

  regex_t r_type;
  regcomp(&r_type, "%TYPE_([0-9])%", REG_EXTENDED);

  FILE *template;
  char tmp[BUFSIZE];
  regmatch_t matches[10];
  int i;
  template=fopen("config.template", "r");
  while(fgets(row, BUFSIZE, template)) {
    if(!strncmp("%SETUP%", row, 7)) {
      write_files_setup();
    }
    else if(!regexec(&r_type, row, 2, matches, 0)) {
      strncpy(tmp, row+matches[1].rm_so, matches[1].rm_eo-matches[1].rm_so);
      tmp[matches[1].rm_eo-matches[1].rm_so]='\0';

      write_files_parse(atoi(tmp));
    }
    else {
      printf("%s", row);
    }
  }
}
