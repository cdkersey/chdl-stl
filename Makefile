PREFIX ?= /usr/local
CXXFLAGS = -std=c++14 -g
LDLIBS = -pthread -lchdl
SOFLAGS = -fPIC -shared

HEADERS = queue.h stack.h map.h initmem.h ag.h dir.h net.h counter.h lfsr.h \
          hash.h bloom.h memreq.h un.h numeric.h sort.h rtl.h alg.h
SRC = rtl.cpp
TESTS = test-queue test-stack test-map test-initmem test-ag test-net \
        test-counter test-lfsr test-hash test-bloom test-memreq test-numeric \
        test-sort test-rtl test-alg
TEST_OUT = test-queue.out test-stack.out test-map.out test-initmem.out \
           test-ag.out test-net.out test-counter.out test-lfsr.out \
           test-hash.out test-bloom.out test-memreq.out test-numeric.out \
           test-sort.out test-rtl.out test-alg.vcd
TEST_LDLIBS=-L./ -lchdl-stl

all : $(TESTS) libchdl-stl.so

test : $(TEST_OUT)

%.out : % libchdl-stl.so
	LD_LIBRARY_PATH=./ ./$< > $@

install: libchdl-stl.so
	mkdir -p $(PREFIX)/include/chdl
	cp $(HEADERS) $(PREFIX)/include/chdl/
	cp libchdl-stl.so $(PREFIX)/lib

%.so : $(SRC) $(HEADERS)
	$(CXX) $(CXXFLGAS) $(LDFLAGS) $(SOFLAGS) -o $@ $< $(LDLIBS)

% : %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS) $(TEST_LDLIBS)

clean:
	rm -f *~ $(TESTS) $(TEST_OUT) *.vcd libchdl-stl.so
