CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = expense_splitter
SRC = main.c ledger.c people.c input.c names_file.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
