#####	arp_spoof	#####
# Author: Maxim "verzhak" Akinin
# E-mail: verzhak@gmail.com
# Date: 04.07.2009
# License: GPLv3

RELEASE = build/arp_spoof
DEBUG = build/arp_spoof-debug

MAIN = main.c
COMPILER = gcc
FLAGS = -Wall
DEBUG_FLAGS = -ggdb3
RELEASE_FLAGS = -O2 -Werror
STAR = "*"
SIMPLE_TEST = simple_test.sh

all: clean
	$(COMPILER) $(FLAGS) $(RELEASE_FLAGS) $(MAIN) -o $(RELEASE)
	echo -e "#!/usr/bin/bash\n/usr/bin/time --format=%S-%U $(RELEASE) $$$(STAR)" > $(SIMPLE_TEST)
	chmod u+x $(SIMPLE_TEST)

debug: clean
	$(COMPILER) $(FLAGS) $(DEBUG_FLAGS) $(MAIN) -o $(DEBUG)
	echo -e "#!/usr/bin/bash\n/usr/bin/time --format=%S-%U $(DEBUG) $$$(STAR)" > $(SIMPLE_TEST)
	chmod u+x $(SIMPLE_TEST)

clean:
	rm -f $(RELEASE) $(DEBUG) $(SIMPLE_TEST)
