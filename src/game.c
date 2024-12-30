#include "leddrv.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>

// Defines the Speed of the Game. Lower Number equals faster movement of the snake
#define REFRESH_DELAY 25

typedef struct {
    int x;
    int y;
} Position;

static Position snake[100];
static int snake_length;
static Position food;

static void draw_pixel(int x, int y, int state, uint16_t *fb) {
    if (x < 0 || x >= LED_COLS || y < 0 || y >= LED_ROWS) return;
    if (state) {
        fb[x] |= (1 << y);
    }
}

static void display_text(uint16_t *fb,char* first, char* second) {
    // Clear Screen and Display "Game Over"
    clear_screen(fb);
    fb_displays(first, sizeof(first), 10, 2, fb);
    DelayMs(500);
    fb_displays(second, sizeof(second), 10, 2, fb);
    DelayMs(500);
    return;
}

static int check_collision(Position pos) {
    // Check collision with walls
    if (pos.x < 0 || pos.x >= LED_COLS || pos.y < 0 || pos.y >= LED_ROWS) {
        return 1;
    }
    // Check collision with itself
    for (int i = 1; i < snake_length; i++) {
        if (snake[i].x == pos.x && snake[i].y == pos.y) {
            return 1;
        }
    }
    return 0;
}

static int is_free_space_available() {
    for (int x = 0; x < LED_COLS; x++) {
        for (int y = 0; y < LED_ROWS; y++) {
            Position pos = {x, y};
            if (!check_collision(pos)) {
                // Free space available
                return 1;
            }
        }
    }
    // No free space available
    return 0;
}

static void generate_food(uint16_t *fb) {
    // Check if there is any free space available
    if (!is_free_space_available()) {
        // Display "You Won" message 2 Times
        for (int i = 0; i < 2; i++) {
            display_text(fb, "You", "Won");
        }
        return;
    }

    // Generate food at a random position
    while (1) {
        food.x = rand() % LED_COLS;
        food.y = rand() % LED_ROWS;
        if (!check_collision(food)) {
            return;
        }
    }
}

static int move_snake(uint16_t *fb) {
    Position new_head = snake[0];
    Position possible_positions[4];
    int num_possible_positions = 0;

    // Check all four possible directions
    Position up = {new_head.x, new_head.y - 1};
    Position down = {new_head.x, new_head.y + 1};
    Position left = {new_head.x - 1, new_head.y};
    Position right = {new_head.x + 1, new_head.y};

    if (!check_collision(up)) possible_positions[num_possible_positions++] = up;
    if (!check_collision(down)) possible_positions[num_possible_positions++] = down;
    if (!check_collision(left)) possible_positions[num_possible_positions++] = left;
    if (!check_collision(right)) possible_positions[num_possible_positions++] = right;

    // Check if no possible positions are available
    if (num_possible_positions == 0) {
        // Display "Game Over" message 2 Times
        for (int i = 0; i < 2; i++) {
            display_text(fb, "Game", "Over");
        }
        return 0;
    }

    // Choose the best direction towards food
    Position best_position = possible_positions[0];
    int min_distance = abs(food.x - best_position.x) + abs(food.y - best_position.y);

    for (int i = 1; i < num_possible_positions; i++) {
        int distance = abs(food.x - possible_positions[i].x) + abs(food.y - possible_positions[i].y);
        if (distance < min_distance) {
            best_position = possible_positions[i];
            min_distance = distance;
        }
    }

    new_head = best_position;

    for (int i = snake_length - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = new_head;
       

    // Check for collision with food
    if (snake[0].x == food.x && snake[0].y == food.y) {
        if (snake_length <= 100){
            snake_length++;
            snake[snake_length-1] = snake[snake_length-2];
            generate_food(fb);
        }
        else {
            // Display game over message 2 Times
            for (int i = 0; i < 2; i++) {
                display_text(fb, "You", "Won");
            }
            return 0;
        }
    }

    // Draw the snake
    for (int i = 0; i < snake_length; i++) {
        draw_pixel(snake[i].x, snake[i].y, 1, fb);
    }

    return 1;
}

void init_game(uint16_t *fb){
    // Initialize snake
    snake_length = 1;
    snake[0].x = LED_COLS / 2;
    snake[0].y = LED_ROWS / 2;

    generate_food(fb);
}

void run_game(uint16_t *fb) {
    clear_screen(fb);
    // Game loop
    draw_pixel(food.x, food.y, 1, fb);
    int run = move_snake(fb);
    DelayMs(REFRESH_DELAY);
    if (run == 0){
        // Restart Game on "Game Over"
        init_game(fb);
    }
}