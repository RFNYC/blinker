TARGET = sensor-example
SRC = main.cpp
CXX = g++
CXXFLAGS = -Wall -std=c++17
LIBS = -lgpiodcxx

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)