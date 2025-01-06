set -e

cc lextlinux.c ../src/extlinux.c -shared -fPIC -o lextlinux.so \
	-iquote../include \
	$(pkg-config --cflags lua5.4) \
	-Wall -Werror -pedantic -Wextra
lua5.4 extlinux.lua
