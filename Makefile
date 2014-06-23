bin/convert : src/*
	mkdir -p bin
	gcc src/io.c src/convert.c -o bin/convert -O `GraphicsMagickWand-config --cppflags --ldflags --libs` -std=c99
