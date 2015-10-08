SRC=$(wildcard src/convert.c)

bin/convert: $(SRC) Makefile
	mkdir -p bin
	gcc ./src/convert.c ./src/io.c -o bin/convert -larchive -ljpeg -Lsrc `pkg-config vips --cflags --libs` 

clean:
	rm -f bin/convert
