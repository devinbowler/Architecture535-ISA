CC = gcc
CFLAGS = -Wall -Wextra -O2
SRC = loop.c memory.c utilities.c
OBJ = $(SRC:.c=.o)
TARGET = ARCH-16

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)