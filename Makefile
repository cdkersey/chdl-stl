PREFIX ?= /usr/local
CXXFLAGS = -std=c++11 -g
LDLIBS = -lchdl

HEADERS = queue.h stack.h map.h initmem.h ag.h dir.h net.h counter.h lfsr.h \
          hash.h bloom.h memreq.h un.h numeric.h
TESTS = test-queue test-stack test-map test-initmem test-ag test-net \
        test-counter test-lfsr test-hash test-bloom test-memreq test-numeric
TEST_OUT = test-queue.out test-stack.out test-map.out test-initmem.out \
           test-ag.out test-net.out test-counter.out test-lfsr.out \
           test-hash.out test-bloom.out test-memreq.out test-numeric.out

all : $(TESTS)

test : $(TEST_OUT)

%.out : %
	./$< > $@

install:
	cp $(HEADERS) $(PREFIX)/include/chdl/

% : %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  -o $@ $< $(LDLIBS)

clean:
	rm -f *~ $(TESTS) $(TEST_OUT) *.vcd
