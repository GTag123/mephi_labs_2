SOURCES := $(wildcard *.cpp)
RESULT := main
SCRIPTS := $(wildcard *.sh)
SCRIPT_TARGETS := $(SCRIPTS:.sh=_run)

OBJECTS := $(SOURCES:.cpp=.o)
CFLAGS := -g -fsanitize=thread -std=c++2a -Wall -Werror
LDFLAGS := -fsanitize=thread

all: run

run: compile
	./$(RESULT)

%_run: %.sh
	/bin/bash $<

compile: $(SOURCES) $(RESULT) $(SCRIPT_TARGETS)

.cpp.o: $(wildcard *.h)
	$(CXX) -c $(CFLAGS) $< -o $@

$(RESULT): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(RESULT)

clean:
	rm -f $(OBJECTS) $(RESULT)

