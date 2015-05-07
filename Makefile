SRC=$(wildcard src/*.c)

bin/convert: $(SRC) Makefile
	mkdir -p bin
	gcc $(SRC) -o bin/convert -L/usr/local/lib -larchive -O `GraphicsMagickWand-config --cflags --cppflags --ldflags --libs` 

clean:
	rm -f bin/convert
