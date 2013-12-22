#!/bin/bash

echo -en "\033[31mrunning aclocal...\033[0m\n\n"
aclocal
echo -en "\n\n"

echo -en "\033[31mrunning autoheader...\033[0m\n\n"
autoheader
echo -en "\n\n"

echo -en "\033[31mrunning autoconf...\033[0m\n\n"
autoconf
echo -en "\n\n"

echo -en "\033[31mrunning libtoolize...\033[0m\n\n"
libtoolize --automake
echo -en "\n\n"

echo -en "\033[31mrunning automake...\033[0m\n\n"
automake -f -c -a
echo -en "\n\n"

echo -en "\033[31mconfigure\033[0m\n\n"
./configure "$@"
