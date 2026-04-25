CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
LDFLAGS =
SRCDIR = src
INCDIR = include
BINDIR = bin
OBJDIR = obj

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/sonnx-compiler

.PHONY: all clean test

all: $(BINDIR) $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete!"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning...";
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean complete!"

test: $(TARGET)
	@echo "Running tests..."
	@echo "=================="
	$(TARGET) -a -c test/test1.txt
	@echo ""
	@echo "=================="

test-lexer: $(TARGET)
	@echo "Testing Lexical Analysis..."
	$(TARGET) -t test/test1.txt

test-parser: $(TARGET)
	@echo "Testing Syntax Analysis..."
	$(TARGET) -p test/test1.txt

test-semantic: $(TARGET)
	@echo "Testing Semantic Analysis..."
	$(TARGET) -a test/test1.txt

test-tac: $(TARGET)
	@echo "Testing TAC Generation..."
	$(TARGET) -c test/test1.txt

rebuild: clean all

help:
	@echo "S-ONNX Compiler Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the compiler (default)"
	@echo "  clean     - Remove all generated files"
	@echo "  test      - Run tests with all options enabled"
	@echo "  test-lexer    - Test lexical analysis only"
	@echo "  test-parser   - Test syntax analysis only"
	@echo "  test-semantic - Test semantic analysis only"
	@echo "  test-tac     - Test TAC generation only"
	@echo "  rebuild   - Clean and rebuild"
	@echo "  help      - Display this help message"
