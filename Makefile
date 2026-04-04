CC = clang
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include -O2
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

SRC = src/main.c src/game.c src/player.c src/world.c src/weapon.c src/enemy.c src/hud.c src/audio.c src/lander.c
OBJ = $(SRC:.c=.o)
TARGET = mondfall

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

models:
	python3 scripts/gen_models.py

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean run models
