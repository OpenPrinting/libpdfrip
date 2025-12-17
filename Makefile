# Programs
CC = gcc
RM = rm -f

# --- Flags ---
# -I adds directories to the include path so the compiler can find headers
# source/pdf             -> for pdfops-private.h
# source/cairo           -> for cairo_device.h
# source/tools/pdf2cairo -> for interpreter.h
CFLAGS   = -g -Wall -Isource/pdf -Isource/cairo -Isource/tools/pdf2cairo
LDFLAGS  =

# --- pkg-config Dependencies ---
PDFIO_CFLAGS   = $(shell pkg-config --cflags pdfio)
PDFIO_LIBS     = $(shell pkg-config --libs pdfio)

CAIRO_CFLAGS   = $(shell pkg-config --cflags cairo-ft cairo freetype2)
CAIRO_LIBS     = $(shell pkg-config --libs cairo-ft cairo freetype2) -lpng16 -lz -lm -lcairo -ljpeg

# --- Final Build Flags ---
BUILD_CFLAGS = $(CFLAGS) $(PDFIO_CFLAGS) $(CAIRO_CFLAGS)
BUILD_LIBS   = $(LDFLAGS) $(PDFIO_LIBS) $(CAIRO_LIBS)

# --- Files ---
# 1. The Main Driver & Logic (in source/tools/pdf2cairo)
SRCS_TOOL  = source/tools/pdf2cairo/pdf2cairo.c 

# 2. The Cairo Backend (in source/cairo)
SRCS_CAIRO = source/cairo/cairo_device.c \
             source/cairo/cairo_path.c \
             source/cairo/cairo_state.c \
             source/cairo/cairo_text.c

# 3. The PDF Operations (in source/pdf)
SRCS_PDF   = source/pdf/pdfops.c \
             source/tools/pdf2cairo/interpreter.c

# Combine all sources
SRCS = $(SRCS_TOOL) $(SRCS_CAIRO) $(SRCS_PDF)
OBJS = $(SRCS:.c=.o)
BIN  = pdf2cairo

# --- Targets ---

.PHONY: all clean

all: $(BIN)
	@echo "Build complete. Executable: $(BIN)"

$(BIN): $(OBJS)
	@echo "Linking $@"
	$(CC) -o $@ $(OBJS) $(BUILD_LIBS)

# Generic rule for compiling C files to Object files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(BUILD_CFLAGS) -c -o $@ $<

clean:
	@echo "Cleaning build files..."
	$(RM) $(OBJS) $(BIN)
