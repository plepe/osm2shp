#include <stdio.h>

void main() {
  FILE *f;
  char row[1024];
  f=fopen("osm2shp.cfg", "r");

  while(fgets(row, 1024, f)) {
    printf("Read: %s", row);
  }
}
