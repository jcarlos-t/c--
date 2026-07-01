CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
TARGET = c--

.PHONY: all clean

all: $(TARGET)
	rm -f $(OBJS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
