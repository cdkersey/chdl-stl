CHDL_PREFIX ?= /usr/local
CXXFLAGS = -std=c++11 -g
LDLIBS = -lchdl

HEADERS = queue.h stack.h map.h initmem.h
TESTS = test-queue test-stack test-map test-initmem
TEST_OUT = test-queue.out test-stack.out test-map.out test-initmem.h

all : $(TESTS)

test : $(TEST_OUT)

%.out : %
	./$< > $@

install:
	cp $(HEADERS) $(CHDL_PREFIX)/include/chdl/

% : %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS)$(LDFLAGS)  -o $@ $< $(LDLIBS)

clean:
	rm -f *~ $(TESTS) $(TEST_OUT) *.vcd
