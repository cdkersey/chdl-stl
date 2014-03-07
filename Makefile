CHDL_PREFIX ?= /usr/local
CXXFLAGS = -std=c++11 -O3
LDLIBS = -lchdl

HEADERS = queue.h stack.h map.h initmem.h ag.h
TESTS = test-queue test-stack test-map test-initmem test-ag
TEST_OUT = test-queue.out test-stack.out test-map.out test-initmem.out \
           test-ag.out

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
