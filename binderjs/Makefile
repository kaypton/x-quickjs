


all: bdjs.c binderjs.c
	clang -I../ -L../../test -lquickjs -lquickjs.lto -lc -o bdjs bdjs.c binderjs.c

test: all
	./bdjs ../binderjs_test

clean:
	@rm -f *.o bdjs


