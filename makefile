CXX = g++
CXXFLAGS = -Wall -Wextra -Iinclude

SRC_DIR = src
INC_DIR = include

# target executable name
TARGET = shell

# source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)