


CXXFLAGS += -std=c++11 -pthread -I ./ -O3
TARGET=MyBot

.PHONY: all
all: $(TARGET)

build : MyBot


run : MyBot
	./MyBot

MyBot: MyBot.cpp
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) 

.PHONY: install
install:
	install -m 0755 halite $(INSTALL_PATH)/bin

