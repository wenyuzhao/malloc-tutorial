CC       = clang
CFLAGS   = -fPIC -Wall -Wextra -Werror -std=gnu17 -pedantic
LIBFLAGS = -shared
MALLOC   = mymalloc
ODIR	 = ./out
LIBTESTFLAGS = -L./out

ifdef RELEASE
CFLAGS += -O3
else
CFLAGS += -g -ggdb3
endif

ifdef LOG
CFLAGS += -DENABLE_LOG
endif

ALL_TESTS_SRC = $(wildcard tests/*.c)
ALL_TESTS = $(ALL_TESTS_SRC:%.c=%)

all: mymalloc

mymalloc: $(MALLOC).c | $(ODIR)/
	@$(CC) $(CFLAGS) $(LIBFLAGS) -o $(ODIR)/lib$(MALLOC).so $<

mymalloc32: $(MALLOC).c | $(ODIR)/
	@$(CC) $(CFLAGS) $(LIBFLAGS) -m32 -o $(ODIR)/lib$(MALLOC)32.so $<

tests/%: _force *.h tests/%.c | mymalloc
	@$(CC) $(CFLAGS) $(LIBTESTFLAGS) $@.c -l$(MALLOC) -o $@ -Wl,-rpath=`pwd`/$(ODIR)

tests/m32: _force *.h tests/m32.c | mymalloc32
	@$(CC) $(CFLAGS) $(LIBTESTFLAGS) -m32 $@.c -l$(MALLOC)32 -o $@ -Wl,-rpath=`pwd`/$(ODIR)

tests/%_: tests/%
	$^

test: $(ALL_TESTS)

$(ODIR)/:
	mkdir -p $(ODIR)

clean:
	rm -f *.o $(ODIR)/*.o
	rm -f *.so $(ODIR)/*.so
	@for test in $(ALL_TESTS); do        \
		echo "rm $$test";            \
		rm -f $$test;      	     \
	done

_force:

.PHONY: clean _force all mymalloc
