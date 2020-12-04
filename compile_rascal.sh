#!/bin/bash
gcc -ggdb -o rascal -Wall -fcommon lib/strlib.c error.c obj.c env.c eval.c txtio.c builtin.c rascal.c
