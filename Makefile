# 1. Variables to make changes easy later
CXX = g++
CXXFLAGS = -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -O2 -Iinclude

# Automatically finds all .cpp files
SRC = $(wildcard *.cpp)

# Automatically changes the .cpp extension to .o for the object list
OBJS = $(patsubst %.cpp, %.o, $(SRC))

# Generate individual executable names (e.g. test.cpp -> test)
BINS = $(SRC:.cpp=)

# 2. The default rule executed when you type 'make'
all: $(OBJS) $(BINS)

# 4. Rule to compile .cpp source files into temporary .o object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 4b. turn each individual .o file into valid standalone executable
%: %.o
	$(CXX) $(CXXFLAGS) -o $@ $<
	rm -f $(OBJS)

# 5. Rule to clean up compiled files to start fresh
clean:
	rm -f $(OBJS)

