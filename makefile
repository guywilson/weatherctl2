###############################################################################
#                                                                             #
# MAKEFILE for wctl2                                                          #
#                                                                             #
# (c) Guy Wilson 2023                                                         #
#                                                                             #
###############################################################################

# Version number for WCTL
MAJOR_VERSION = 0
MINOR_VERSION = 1

# Directories
SOURCE = src
BUILD = build
DEP = dep

# What is our target
TARGET = wctl2

# Tools
VBUILD = vbuild
CPP = g++
C = gcc
LINKER = g++

# postcompile step
PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
# postcompile step
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CPPFLAGS = -c -Wall -pedantic -std=c++11 -I/Users/guy/Library/include
CFLAGS = -c -Wall -pedantic -I/Users/guy/Library/include
#MGFLAGS=-DMG_ENABLE_SSL
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

# Libraries
STDLIBS = -pthread -lstdc++
EXTLIBS = -lstrutils

COMPILE.cpp = $(CPP) $(CPPFLAGS) $(DEPFLAGS) $(MGFLAGS) -o $@
COMPILE.c = $(C) $(CFLAGS) $(DEPFLAGS) -o $@
LINK.o = $(LINKER) -L/Users/guy/Library/lib $(STDLIBS) -o $@

CSRCFILES = $(wildcard $(SOURCE)/*.c)
CPPSRCFILES = $(wildcard $(SOURCE)/*.cpp)
OBJFILES = $(patsubst $(SOURCE)/%.c, $(BUILD)/%.o, $(CSRCFILES)) $(patsubst $(SOURCE)/%.cpp, $(BUILD)/%.o, $(CPPSRCFILES))
DEPFILES = $(patsubst $(SOURCE)/%.c, $(DEP)/%.d, $(CSRCFILES)) $(patsubst $(SOURCE)/%.cpp, $(DEP)/%.d, $(CPPSRCFILES))

all: $(TARGET)

# Compile C/C++ source files
#
$(TARGET): $(OBJFILES)
	$(LINK.o) $^ $(EXTLIBS)

$(BUILD)/%.o: $(SOURCE)/%.c
$(BUILD)/%.o: $(SOURCE)/%.c $(DEP)/%.d
	$(PRECOMPILE)
	$(COMPILE.c) $<
	$(POSTCOMPILE)

$(BUILD)/%.o: $(SOURCE)/%.cpp
$(BUILD)/%.o: $(SOURCE)/%.cpp $(DEP)/%.d
	$(PRECOMPILE)
	$(COMPILE.cpp) $<
	$(POSTCOMPILE)

.PRECIOUS = $(DEP)/%.d
$(DEP)/%.d: ;

-include $(DEPFILES)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin

version:
	$(VBUILD) -incfile wctl.ver -template version.c.template -out $(SOURCE)/version.c -major $(MAJOR_VERSION) -minor $(MINOR_VERSION)

clean:
	rm -r $(BUILD)
	rm -r $(DEP)
	rm $(TARGET)
