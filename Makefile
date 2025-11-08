# Programs
CC = gcc
RM = rm -f

# Base flags
# CFLAGS: Flags for C compiler (g=debug, Wall=all warnings)
# LDFLAGS: Flags for linker
CFLAGS   = -g -Wall
LDFLAGS  =

# --- pkg-config Dependencies ---

# Get compiler and linker flags for the required libraries
# PDFIO_CFLAGS: Includes like -I/usr/local/include/pdfio
# PDFIO_LIBS:   Libs like -L/usr/local/lib -lpdfio
PDFIO_CFLAGS   = $(shell pkg-config --cflags pdfio)
PDFIO_LIBS     = $(shell pkg-config --libs pdfio)

# *** UPDATED SECTION ***
# Get *both* CFLAGS (includes) and LIBS (linker flags) for Cairo/Freetype
CAIRO_CFLAGS   = $(shell pkg-config --cflags cairo-ft cairo freetype2)
CAIRO_LIBS     = $(shell pkg-config --libs cairo-ft cairo freetype2) -lpng16 -lz -lm -lcairo -ljpeg

# --- Final Build Flags ---

# Combine base flags with library-specific flags
# *** UPDATED LINE: Added $(CAIRO_CFLAGS) ***
BUILD_CFLAGS = $(CFLAGS) $(PDFIO_CFLAGS) $(CAIRO_CFLAGS)
BUILD_LIBS   = $(LDFLAGS) $(PDFIO_LIBS) $(CAIRO_LIBS)

# --- Project Files ---

# Source files for the pdf2cairo executable
PDF2CAIRO_DIR  = pdf2cairo
PDF2CAIRO_SRCS = $(wildcard $(PDF2CAIRO_DIR)/*.c)
PDF2CAIRO_OBJS = $(PDF2CAIRO_SRCS:.c=.o)
PDF2CAIRO_BIN  = $(PDF2CAIRO_DIR)/pdf2cairo_main

# Source file for the test program
TESTPDF2CAIRO_SRC = testpdf2cairo.c
TESTPDF2CAIRO_OBJ = $(TESTPDF2CAIRO_SRC:.c=.o)
TESTPDF2CAIRO_BIN = testpdf2cairo

# --- Primary Targets ---

.PHONY: all
all: $(PDF2CAIRO_BIN) $(TESTPDF2CAIRO_BIN)
	@echo "Build complete. Executables are:"
	@echo "  $(PDF2CAIRO_BIN)"
	@echo "  $(TESTPDF2CAIRO_BIN)"

.PHONY: test
test: $(TESTPDF2CAIRO_BIN)
	@echo "Running pdf2cairo renderer tests..."
	./$(TESTPDF2CAIRO_BIN)

.PHONY: clean
clean:
	@echo "Cleaning build files..."
	$(RM) $(PDF2CAIRO_OBJS) $(PDF2CAIRO_BIN)
	$(RM) $(TESTPDF2CAIRO_OBJ) $(TESTPDF2CAIRO_BIN)
	@echo "Cleaning renderer output directory..."
	$(RM) -r testfiles/renderer-output

# --- Build Rules ---

# Rule to build the pdf2cairo_main executable
$(PDF2CAIRO_BIN): $(PDF2CAIRO_OBJS)
	@echo "Linking executable $@"
	$(CC) -o $@ $(PDF2CAIRO_OBJS) $(BUILD_LIBS)

# Rule to build the testpdf2cairo executable
$(TESTPDF2CAIRO_BIN): $(TESTPDF2CAIRO_OBJ)
	@echo "Linking test program $@"
	$(CC) -o $@ $(TESTPDF2CAIRO_OBJ) $(BUILD_LIBS)

# --- Object File Rules ---

# Generic rule to build object files for pdf2cairo
$(PDF2CAIRO_DIR)/%.o: $(PDF2CAIRO_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(BUILD_CFLAGS) -c -o $@ $<

# Generic rule to build object file for the test
$(TESTPDF2CAIRO_OBJ): $(TESTPDF2CAIRO_SRC)
	@echo "Compiling $<..."
	$(CC) $(BUILD_CFLAGS) -c -o $@ $<
