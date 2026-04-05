CC = clang
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include -Ivendor/flecs -O2
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo

SRC = src/main.c src/game.c src/game_intro.c src/player.c src/world.c src/world/world_noise.c src/world/world_draw.c src/weapon.c src/weapon/weapon_sound.c src/weapon/weapon_draw.c src/enemy/enemy_draw.c src/enemy/enemy_draw_death.c src/enemy/enemy_components.c src/enemy/enemy_spawn.c src/enemy/enemy_systems.c src/enemy/enemy_ai.c src/enemy/enemy_physics.c src/enemy/enemy_death_systems.c src/enemy/enemy_morale.c src/enemy/enemy_draw_ecs.c src/hud.c src/hud/hud_primitives.c src/hud/hud_fuehrerauge.c src/audio.c src/lander.c src/pickup.c src/combat_ecs.c src/sound_gen.c src/structure/structure.c src/structure/structure_draw_exterior.c src/structure/structure_draw_interior.c src/structure/structure_draw_furniture.c src/ecs_world.c
OBJ = $(SRC:.c=.o) vendor/flecs/flecs.o
TARGET = mondfall

all: $(TARGET) test

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

vendor/flecs/flecs.o: vendor/flecs/flecs.c
	$(CC) -std=c99 -O2 -Ivendor/flecs -c $< -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/world/%.o: src/world/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/enemy/%.o: src/enemy/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/weapon/%.o: src/weapon/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/structure/%.o: src/structure/%.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

src/hud/%.o: src/hud/%.c
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
	rm -f $(OBJ) vendor/flecs/flecs.o $(TARGET) $(TEST_TARGET)

.PHONY: all clean run models test
