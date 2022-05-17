CC = g++

CFLAGS = -Wall -Wextra -Wpedantic -no-pie -g

SRC = lib/Text/TextAnalyzer.cpp Translator/TranslatorFunctions.cpp Translator/JIT.cpp

progName = jit

all:
	$(CC) $(CFLAGS) $(SRC) -o $(progName)

