eepasm: eepasm.cpp parsing_utils.cpp eepasm.h
	g++ eepasm.cpp parsing_utils.cpp -o eepasm

all: eepasm

.PHONY: all
