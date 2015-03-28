CC=`which gcc`
TPL_GEN=`which python3` bin/gen_tpl.py
RM=`which rm`
SPWNCGI=`which spawn-fcgi`
KILL=`which kill

CFLAGS=-Wall -Werror -Wextra --std=c11 -D_GNU_SOURCE
LDFLAGS=-lm -lpthread -lfcgi

APP=fastpoll
PID=/var/run/$(APP).pid

SRC=$(shell find src -name '*.[c]')
OBJ=$(SRC:%.c=%.o)

TPL_IN=$(shell find src -name '*.td')
TPL_OUT=src/template_def.h

.PHONY: clean tpl start stop

all: tpl app

app: $(OBJ)
	$(CC) -o bin/$(APP) $(OBJS) $(LDFLAGS)

tpl:
	$(TPL_GEN) -i $(TPL_IN) -o $(TPL_OUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	$(RM) -f $(OBJ)

# ---------
# debugging
 
start:
	$(SPWNCGI) -p 9000 -a 127.0.0.1 -f bin/$(APP) -P $(PID)
 
stop:
	$(KILL) -9 $$(cat $(PID))