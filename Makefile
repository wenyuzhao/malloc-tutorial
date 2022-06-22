CC      = clang
CFLAGS  = -fPIC -Wall -Wextra -Werror -std=gnu17 -pedantic
MALLOC  = mymalloc

ifdef RELEASE
CFLAGS += -O3
else
CFLAGS += -g -ggdb3
endif

ifdef LOG
CFLAGS += -DENABLE_LOG
endif

tests/%: _force *.h tests/%.c $(MALLOC).c
	$(CC) $(CFLAGS) $@.c $(MALLOC).c -o $@

ALL_TESTS = tests/simple

test: $(ALL_TESTS)
	@for test in $(ALL_TESTS); do    \
		echo "> $$test";             \
		$$test;                      \
	done

clean:
	rm -f *.o
	rm -f *.so
	@for test in $(ALL_TESTS); do    \
		echo "rm $$test";            \
		rm -f $$test;                \
	done

_force:

.PHONY: clean _force