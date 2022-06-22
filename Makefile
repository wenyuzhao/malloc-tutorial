CC      = clang
CFLAGS  = -I. -fPIC -Wall -Wextra -Werror -std=c11 -pedantic
MALLOC  = mymalloc
TARGET = $(MALLOC).so

ifdef RELEASE
CFLAGS += -O3
else
CFLAGS += -DDEBUG -g -ggdb3
endif

ifdef LOG
CFLAGS += -DENABLE_LOG
endif

$(TARGET): *.h
	$(CC) -o $(TARGET) $(MALLOC).c $(CFLAGS) -shared

tests/%: tests/%.c
	$(CC) $@.c -o $@

ALL_TESTS = tests/simple

test: $(TARGET) $(ALL_TESTS)
	@for test in $(ALL_TESTS); do               \
		echo "LD_PRELOAD=./$(TARGET) $$test";   \
		LD_PRELOAD=./$(TARGET) $$test;          \
	done

clean:
	rm *.o
	rm *.so
	@for test in $(ALL_TESTS); do               \
		echo "rm $$test";                       \
		rm $$test;                              \
	done

.PHONY: clean $(TARGET)