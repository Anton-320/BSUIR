#makefile
CC = gcc

LDLIBS = -lncurses -lc -lrt # Данные флаги обязательны, lc - для функций <endian.h> (стандартная библиотека)
CFLAGS = -g -ggdb -pedantic -W -Wall -Wextra


.SUFFIXES:   
.SUFFIXES: .c .o

DEBUG   = ./build/linux/debug
RELEASE = ./build/linux/release
OUT_DIR = $(DEBUG)
vpath %.c src
vpath %.h src
vpath %.o build/linux/debug

ifeq ($(MODE), release)
  CFLAGS = -pedantic -W -Wall -Wextra -Werror
  OUT_DIR = $(RELEASE)
  vpath %.o build/linux/release
endif

objects =  $(OUT_DIR)/main.o $(OUT_DIR)/fs_check.o $(OUT_DIR)/general_fun.o $(OUT_DIR)/io.o $(OUT_DIR)/lfn.o

prog = $(OUT_DIR)/fatcheck

all: $(prog) 

$(prog) : $(objects) 
	$(CC) $(CFLAGS) $(objects) -o $@ $(LDLIBS)

$(OUT_DIR)/%.o : %.c
	$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean 
clean:
	@rm -rf $(DEBUG)/* $(RELEASE)/* test
