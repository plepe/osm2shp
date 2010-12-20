osm2shp: osm2shp.c config.c
	cc -g -o osm2shp osm2shp.c -DLARGEFILE_SOURCE -lshp -lxml2 -I /usr/include/libxml2 -I /usr/include/glib-2.0 -lglib-2.0 -I /usr/lib/glib-2.0/include 

mkconfig: mkconfig.c
	cc -o mkconfig mkconfig.c

config.c: mkconfig osm2shp.cfg
	./mkconfig
