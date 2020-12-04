#!/bin/bash
gcc -o rascal -Wall -fcommon lib/strlib.c error.c obj.c env.c eval.c txtio.c builtin.c rascal.c
