
CXX = g++
CXXFLAGS = -std=gnu++17
LDFLAGS = -lboost_system

ifeq ($(OS),Windows_NT)
    CXXFLAGS += -DWIN32
	LDFLAGS += -lws2_32
	TARGET = test.exe
else
	TARGET = test
endif

.PHONY: default build clean run

default: build

build: $(TARGET)

clean:
	rm -vf *.o $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): test.cpp ../include/async.hpp
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)
