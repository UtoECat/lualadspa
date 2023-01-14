#!/bin/bash

gcc luajack.c -Wall -Wextra '-lluajit-5.1' -lm -ljack -o luajack
