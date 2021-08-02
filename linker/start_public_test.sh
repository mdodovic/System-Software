#!/bin/bash

./linker -o linker_hex_output.o -hex -place=ivt@0x0000 linker_input_projmain.o linker_input_projinterrupts.o 