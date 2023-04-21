CFLAGS = -O3 -Wall -Werror -Wimplicit-fallthrough
SRCS = $(wildcard src/*.c)
HDRS = $(wildcard src/*.h)
OBJS = $(patsubst src/%.c, obj/%.o, $(SRCS))
CC = clang
CFLAGS += -g

rvemu: $(OBJS)
	$(CC) $(CFLAGS) -lm -o $@ $^ $(LDFLAGS)

$(OBJS): obj/%.o: src/%.c $(HDRS)
	@mkdir -p $$(dirname $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -rf rvemu obj/
