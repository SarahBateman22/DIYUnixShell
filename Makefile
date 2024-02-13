#Makefile - builds DIY Unix Shell Project
#
# to build enter 'make build'
# to run enter 'make run'
# to clear .o and executable files enter 'make clean'
#
# targets:
# build - builds all files
# clean - deletes all .o files

CXX = c++
CXXFLAGS = --std=c++11 -c

# the prerequisite of the run target is the buildFile (executable file)
run: shell
	./shell

build: shell

shell: main.o shelpers.o
	$(CXX) main.o shelpers.o -lreadline -o shell

clean:
	rm -f *.o *.out *.gch shell

main.o: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp

shelpers.o: shelpers.cpp shelpers.hpp
	$(CXX) $(CXXFLAGS) shelpers.cpp shelpers.hpp
