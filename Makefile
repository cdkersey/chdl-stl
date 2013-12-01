CHDL_PREFIX ?= /usr/local
CXXFLAGS = -std=c++11
LDLIBS = -lchdl

HEADERS = queue.h stack.h map.h
TESTS = test-queue test-stack test-map
TEST_OUT = test-queue.out test-stack.out test-map.out

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
