gcc vspace.c \
    -O3 \
    -framework OpenGL \
    -Iglfw-3.3.6.bin.MACOS/include/ \
    -Lglfw-3.3.6.bin.MACOS/lib-x86_64/ \
    -lglfw \
    -o vspace
