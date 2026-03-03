CC = g++
CFLAGS = -Wall -Wextra -O2 -std=c++17

SRCS = mystring.cpp dynarray.cpp hashtable.cpp tokenizer.cpp stemmer.cpp indexer.cpp searcher.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = search_engine

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
