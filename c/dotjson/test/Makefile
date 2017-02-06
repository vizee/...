CC=gcc
CFLAGS=-Wall -ansi -pedantic

src=decode.c encode.c object.c

ifdef ENABLE_COMMENT
src+= comment.c
CFLAGS+= -DDOTJSON_FEATURE_COMMENT
endif

targets=$(src:%.c=%)

all: $(targets)

$(targets): %: %.c ../dotjson.c
	$(CC) $(CFLAGS) -o $@ util.c $<

clean:
	-rm $(targets)

.PHONY: all clean

.IGNORE: clean
