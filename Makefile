.PHONY:install uninstall
CC=gcc
CFLAGS=-O3 -s 
LDFLAGS=-lcurl -ljson-c

PREFIX=/usr/local/bin

TARGET=ponywall
SOURCE=$(TARGET:=.c)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(SOURCE)

install:
	install $(TARGET) $(PREFIX)

uninstall:
	rm -rf $(PREFIX)/$(TARGET)
