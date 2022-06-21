CC      = clang
CFLAGS  = -I. -fPIC -Wall -Wextra -Werror -std=c11 -pedantic -g -ggdb3
TARGET  = mymalloc.so

$(TARGET): mymalloc.o
	$(CC) -o $(TARGET) $^ -shared

tests/%: tests/%.c
	$(CC) $@.c -o $@

ALL_TESTS = tests/simple

test: $(TARGET) $(ALL_TESTS)
	@for test in $(ALL_TESTS); do             \
		echo "LD_PRELOAD=$(TARGET) $$test";   \
		LD_PRELOAD=$(TARGET) $$test;          \
	done