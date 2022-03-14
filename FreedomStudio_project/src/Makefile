# Copyright 2019 SiFive, Inc #
# SPDX-License-Identifier: Apache-2.0 #
# Copyright 2022 Shoei Nashimoto

LDLIBS += -Isep1 -Isep2 -Isep3 -Isep2/micro-aes

OUTDIR := subbuild
SRCS := $(wildcard *.c) $(wildcard *.h) $(wildcard sep*/*.c) $(wildcard sep*/*.h) $(wildcard sep2/*/*.c) $(wildcard sep2/*/*.h)
OBJS := $(addprefix $(OUTDIR)/, $(patsubst %.c,%.o,$(filter %.c,$(SRCS))))

$(OUTDIR)/%.o: %.c
	@if [ ! -e `dirname $@` ]; then mkdir -p `dirname $@`; fi
	$(CC) $(CFLAGS) -o $@ -c $<

$(PROGRAM): $(OBJS) $(SRCS)
	$(CC) $(XCFLAGS) $(CFLAGS) $(LDFLAGS) $(filter %.o,$^) $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f $(PROGRAM) $(PROGRAM).hex
	rm -rf $(OUTDIR)

