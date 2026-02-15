#
# Makefile for libpdfrip
#
# Copyright 2025-2026 Uddhav Phatak
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

# POSIX makefile
.POSIX:


# Build silently
.SILENT:


# Programs and options
CC      = gcc
RM      = rm -f

CFLAGS   = -g -Wall -Isource/pdf -Isource/cairo -Isource/tools/pdf2cairo

# --- pkg-config Dependencies ---
PDFIO_CFLAGS   = $(shell pkg-config --cflags pdfio)
PDFIO_LIBS     = $(shell pkg-config --libs pdfio)

CAIRO_CFLAGS   = $(shell pkg-config --cflags cairo-ft cairo)
CAIRO_LIBS     = $(shell pkg-config --libs cairo-ft cairo) -lpng16 -lz -lm -lcairo -ljpeg

# --- Final Build Flags ---
BUILD_CFLAGS = $(CFLAGS) $(PDFIO_CFLAGS) $(CAIRO_CFLAGS)
BUILD_LIBS   = $(LDFLAGS) $(PDFIO_LIBS) $(CAIRO_LIBS)

# --- Files ---
# 1. The Main Driver & Logic (in source/tools/pdf2cairo)
SRCS_TOOL  = source/tools/pdf2cairo/pdf2cairo.c 

# 2. The Cairo Backend (in source/cairo)
SRCS_CAIRO = source/cairo/cairo-device.c \
             source/cairo/cairo-path.c \
             source/cairo/cairo-state.c \
             source/cairo/cairo-text.c

# 3. The PDF Operations (in source/pdf)
SRCS_PDF   = source/pdf/pdfops.c \
             source/pdf/parser.c \
	     source/pdf/pdf-text.c

# Combine all sources
SRCS = $(SRCS_TOOL) $(SRCS_CAIRO) $(SRCS_PDF)
OBJS = $(SRCS:.c=.o)
BIN  = source/tools/pdf2cairo/pdf2cairo


# --- Targets ---

.PHONY: all clean test valgrind

all: $(BIN)

# Link the final binary
$(BIN): $(OBJS)
	@echo Linking $@...
	$(CC) $(BUILD_CFLAGS) -o $@ $(OBJS) $(BUILD_LIBS)

# Compile source files into object files
.SUFFIXES: .c .o
.c.o:
	@echo Compiling $<...
	$(CC) $(BUILD_CFLAGS) -c -o $@ $<

# Run the test suite
test: testpdf2cairo
	@echo Running tests...
	./testpdf2cairo

# Build the test runner
testpdf2cairo: testpdf2cairo.o $(filter-out source/tools/pdf2cairo/pdf2cairo.o, $(OBJS))
	@echo Linking $@...
	$(CC) $(BUILD_CFLAGS) -o $@ testpdf2cairo.o $(filter-out source/tools/pdf2cairo/pdf2cairo.o, $(OBJS)) $(BUILD_LIBS)

# Run under Valgrind to detect the segfaults and leaks
valgrind: testpdf2cairo
	valgrind --leak-check=full ./testpdf2cairo

# Clean build files
clean:
	@echo Cleaning build files...
	$(RM) $(BIN) $(OBJS) testpdf2cairo testpdf2cairo.o

# --- Dependencies (Manual Header Tracking) ---
$(OBJS): source/pdf/pdfops-private.h source/cairo/cairo-private.h source/pdf/parser.h
testpdf2cairo.o: testpdf2cairo.c test.h
