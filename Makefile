CC=gcc
CFLAGS=-Wall -Werror -Wpedantic -Wextra
LDFLAGS=-lpthread
SOURCE:=$(shell find $(SOURCEDIR) -name '*.c')
OBJECTS:=$(addprefix obj/,$(notdir $(SOURCE:%.c=%.o)))
TARGETS:=$(addprefix bin/,$(notdir $(SOURCE:%.c=%)))
HIDE = @

all: $(TARGETS)
	@echo Build Complete!

$(TARGETS):$(OBJECTS)
	$(HIDE)$(CC) obj/$(notdir $@).o -o $@ $(LDFLAGS)

$(OBJECTS):$(SOURCE)
	$(HIDE)$(CC) $(CFLAGS) -c $(shell find $(SOURCEDIR) -name $(notdir $(@:%.o=%.c))) -o $@ $(LDFLAGS)

clean:
	rm -rf obj/* bin/* data.txt
	reset

rebuild: clean all

.PHONY: clean