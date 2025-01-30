clang vspace.c \
      -O3 \
      -framework Cocoa \
      -framework OpenGL \
      -framework IOKit \
      -Iglfw-3.4.bin.MACOS/include/ \
      -Lglfw-3.4.bin.MACOS/lib-arm64/ \
      -lglfw3 \
      -o vspace
