SRC_DIR = ./src
_SRC = $(shell find ./src -type f -iname "*.c" -exec basename {} \;)
SRC = $(patsubst %,$(SRC_DIR)/%,$(SRC))

ODIR = ./obj
_OBJ = $(_SRC:.c=.o)
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

HEADERS = $(shell find ./include/*.h)

MAPPALIB = $(shell find ../mappaLib/obj/*.o)

O_DIR = ./obj
CC = gcc

CFLAGS = -c

LINK = -lcommons -lpthread -lreadline

OUT_NAME = memoria.out

.PHONY: makeFolder

build: makeMappaLib makeFolder $(OBJ)
	$(CC) main.c -o $(OUT_NAME) $(OBJ) $(MAPPALIB) $(LINK)
	
$(ODIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $< $(LINK)
	
makeMappaLib:
	make -C ../mappaLib
	
makeFolder:
	if [ ! -d ./obj ]; then mkdir obj; fi
	
debug: CFLAGS := $(CFLAGS) -g
debug: build

clean:
	rm -rf obj
	rm -rf *.out
	rm -rf *.dmp
	rm -rf *.bin
	rm -rf memoria.log
	make clean -C ../mappaLib