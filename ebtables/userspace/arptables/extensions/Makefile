#! /usr/bin/make

EXT_FUNC+=standard mangle CLASSIFY
EXT_OBJS+=$(foreach T,$(EXT_FUNC), extensions/arpt_$(T).o)

extensions/ebt_%.o: extensions/arpt_%.c include/arptables.h include/arptables_common.h
	$(CC) $(CFLAGS) $(PROGSPECS) -c -o $@ $<
