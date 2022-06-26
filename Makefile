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

_ALL_TESTS = $(wildcard tests/*.c)
ALL_TESTS = $(_ALL_TESTS:%.c=%)

tests/%: _force *.h tests/%.c $(MALLOC).c
	@$(CC) $(CFLAGS) $@.c $(MALLOC).c -o $@
	$@

tests/m32: _force *.h tests/m32.c $(MALLOC).c
	@$(CC) $(CFLAGS) -m32 $@.c $(MALLOC).c -o $@
	$@

test: $(ALL_TESTS)

clean:
	rm -f *.o
	rm -f *.so
	@for test in $(ALL_TESTS); do    \
		echo "rm $$test";            \
		rm -f $$test;                \
	done

_force:

.PHONY: clean _force