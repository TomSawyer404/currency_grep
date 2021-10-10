CC = gcc
CFLAGS = -c -g -Wall
LDFLAGS = -pthread

DIR_BINS = ./target
DIR_SRCS = ./src

dirs := $(DIR_BINS)
bin = cgrep 
src = $(wildcard $(DIR_SRCS)/*.c)
obj = $(patsubst %.c,%.o,$(src))

bin := $(addprefix $(DIR_BINS)/,$(bin))
#----------------------

default: $(dirs) $(bin)

$(dirs):
	mkdir -p $@

$(bin): $(obj)
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	@if [ -f $(bin) ]; then\
		rm -r $(bin) $(obj);\
		echo "Done cleaning $(bin)...";\
		echo "Done cleaning $(obj)...";\
	else \
		rm -r $(obj);\
		echo "Done cleaning $(obj)...";\
	fi

.PHONY: default clean
