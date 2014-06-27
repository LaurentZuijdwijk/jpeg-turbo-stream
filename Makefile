SRC=$(wildcard src/*.c)

bin/convert: $(SRC) Makefile
	mkdir -p bin; gcc $(SRC) -o bin/convert -O `GraphicsMagickWand-config --cflags --cppflags --ldflags --libs` -std=c99

clean:
	rm -f bin/convert
