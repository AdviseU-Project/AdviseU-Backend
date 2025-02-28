LIBRARY_CC=g++ -std=c++20
LIBRARY_AR=ar
LIBRARY_TARGET=libsolver.a
LIBRARY_OBJECT=solver.o
LIBRARY_SOURCE=solver.cpp
LIBRARY_HEADER=solver.hpp
LIBRARY_CFLAGS=-c -g
LIBRARY_LFLAGS=-L. -lsgecodesearch -lsgecodeint -lsgecodekernel -lsgecodesupport
LIBRARY_AFLAGS=-r
TEST_CC=g++ -std=c++20
TEST_TARGET=main
TEST_SOURCES=main.cpp loader.hpp
TEST_CFLAGS=-static $(pkg-config --cflags --libs libmongocxx)
TEST_LFLAGS=-L. -lsolver -lsgecodesearch -lsgecodeint -lsgecodekernel -lsgecodesupport

# all: $(TEST_TARGET) $(LIBRARY_TARGET)

# library: $(LIBRARY_TARGET)

$(TEST_TARGET): $(TEST_SOURCES) $(LIBRARY_TARGET)
	$(TEST_CC) $(TEST_SOURCES) $(TEST_CFLAGS) -o $(TEST_TARGET) $(TEST_LFLAGS)

# $(LIBRARY_TARGET): $(LIBRARY_OBJECT)
# 	$(LIBRARY_AR) $(LIBRARY_AFLAGS) $(LIBRARY_TARGET) $(LIBRARY_OBJECT)

# $(LIBRARY_OBJECT): $(LIBRARY_SOURCE) $(LIBRARY_HEADER)
# 	$(LIBRARY_CC) $(LIBRARY_SOURCE) $(LIBRARY_CFLAGS) -o $(LIBRARY_OBJECT) $(LIBRARY_LFLAGS)

purge: clean
	rm -f $(TEST_TARGET)
	rm -f $(LIBRARY_TARGET)

clean:
	rm -f *.o