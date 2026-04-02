# Define variables for directories and files
SRC_DIR := ../C
TARGET := rpn_function.js
TARGET_HYBRID := rpn_function_hybrid.js
TARGET_HYBRID_MAIN := rpn_hybrid_main

# Source files for standard version
SOURCES := ConstantRecognition_function2_for_WASM.c \
           $(SRC_DIR)/constant.c \
           $(SRC_DIR)/mathematica.c

# Source files for hybrid FP32+FP64 version (WASM library)
SOURCES_HYBRID := ConstantRecognition_hybrid_for_WASM.c \
                  $(SRC_DIR)/constant.c \
                  $(SRC_DIR)/mathematica.c 

# Source files for hybrid with main (for testing in terminal)
SOURCES_HYBRID_MAIN := ConstantRecognition_main_hybrid_for_WASM.c \
                       $(SRC_DIR)/constant.c \
                       $(SRC_DIR)/mathematica.c

# Compiler and flags
EMCC := emcc
CC := gcc
CFLAGS := -Wall -O2
CFLAGS_HYBRID := -Wall -O3 -ffast-math
CFLAGS_NATIVE := -Wall -O3 -ffast-math -lm

# Standard version flags
EMFLAGS := -s WASM=1 \
           -s EXPORTED_FUNCTIONS='["_search_RPN", "_free"]' \
           -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]'

# Hybrid version flags - single function exported
EMFLAGS_HYBRID := -s WASM=1 \
                  -s EXPORTED_FUNCTIONS='["_search_RPN_hybrid", "_free"]' \
                  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
                  -s ALLOW_MEMORY_GROWTH=1

# Default target - build both versions
all: $(TARGET) $(TARGET_HYBRID)

# Standard version
$(TARGET): $(SOURCES)
	$(EMCC) $(CFLAGS) $(SOURCES) $(EMFLAGS) -o $@

# Hybrid FP32+FP64 version (faster!)
$(TARGET_HYBRID): $(SOURCES_HYBRID)
	$(EMCC) $(CFLAGS_HYBRID) $(SOURCES_HYBRID) $(EMFLAGS_HYBRID) -o $@

# Build only hybrid WASM
hybrid: $(TARGET_HYBRID)

# Build native executable for testing (no emscripten needed)
native: $(TARGET_HYBRID_MAIN)

$(TARGET_HYBRID_MAIN): $(SOURCES_HYBRID_MAIN)
	$(CC) $(CFLAGS_NATIVE) ConstantRecognition_main_hybrid_for_WASM.c \
		$(SRC_DIR)/constant.c $(SRC_DIR)/mathematica.c $(SRC_DIR)/math2.c \
		-o $@ -lm

# Test hybrid with example
test: native
	./$(TARGET_HYBRID_MAIN) 3.14159265 7 0 1
	./$(TARGET_HYBRID_MAIN) 77777 9 0 1

# Clean up
clean:
	rm -f $(TARGET) $(TARGET:.js=.wasm)
	rm -f $(TARGET_HYBRID) $(TARGET_HYBRID:.js=.wasm)
	rm -f $(TARGET_HYBRID_MAIN)
