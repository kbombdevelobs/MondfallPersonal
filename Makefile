CC = clang
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include -O2
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

SRC = src/main.c src/game.c src/player.c src/world.c src/weapon.c src/weapon/weapon_sound.c src/weapon/weapon_draw.c src/enemy/enemy.c src/enemy/enemy_draw.c src/hud.c src/audio.c src/lander.c src/pickup.c src/combat.c src/sound_gen.c
OBJ = $(SRC:.c=.o)
TARGET = mondfall

all: $(TARGET) test

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/enemy/%.o: src/enemy/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/weapon/%.o: src/weapon/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

run: $(TARGET)
	./$(TARGET)

models:
	python3 scripts/gen_models.py

TEST_SRC = tests/test_game.c
TEST_OBJ = $(filter-out src/main.o,$(OBJ))
TEST_TARGET = tests/test_game

test: $(TEST_OBJ) $(TEST_SRC)
	$(CC) $(CFLAGS) -Isrc $(TEST_SRC) $(TEST_OBJ) -o $(TEST_TARGET) $(LDFLAGS)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET)

.PHONY: all clean run models test
