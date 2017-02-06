CC=gcc
CFLAGS=-pthread -I.. -Wall

SOURCE=task.c wait.c redo.c
TARGET=$(SOURCE:%.c=%)

.PHONY: all clean
.IGNORE: clean

all: $(TARGET)

$(TARGET): %: %.c
	$(CC) $(CFLAGS) -o $@ $< ../thrpool.c

clean:
	@rm $(TARGET)

