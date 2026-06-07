#include "leddrv.h"
#include <stdint.h>
#include <stdlib.h>
#include "CH58xBLE_LIB.h"  // for TMOS
#include "font3x5.h"           // for font3x5
#include <string.h>         // for memset
#include "button.h"
#include "auxbtn.h"
#include "CH58x_common.h"
#include "game.h"
#include "data.h"


#define SNAKE_MAX_LEN       100
#define SPEED_INITIAL_MS    300
#define SPEED_MIN_MS        100
#define SPEED_STEP_MS       20
#define MS_TO_TMOS(ms)      ((uint32_t)(ms) * 1000 / 625)
#define GAME_TICK           (1 << 0)

typedef enum { 
    DIR_UP=0, DIR_RIGHT, DIR_DOWN, DIR_LEFT 
} Direction;

static tmosTaskID  game_taskid = INVALID_TASK_ID;
static uint16_t   *game_fb;
static Direction   cur_dir, next_dir;
static uint16_t    score;
static int         speed_ms;

extern void return_to_menu(void);
typedef struct {
    int x;
    int y;
} Position;

static void game_exit(void);
static Position snake[100];
static int snake_length;
static Position food;

static void draw_pixel(int x, int y, int state) {
    if (x < 0 || x >= LED_COLS || y < 0 || y >= LED_ROWS) return;
    if (state) {
        game_fb[x] |= (1 << y);
    }
    else game_fb[x]&= ~(1 << y);
}

static void on_turn_left(void)  { next_dir = (cur_dir + 3) % 4; }
static void on_turn_right(void) { next_dir = (cur_dir + 1) % 4; }

static void game_putchar(char c, int col, int row)
{
    for (int i = 0; i < 4; i++) {
        if (col + i >= LED_COLS) break;
        game_fb[col + i] = (game_fb[col + i] & ~(0x1f << row))
                         | (font3x5[c - ' '][i] << row);
    }
}

static void game_puts(const char *s, int col, int row)
{
    while (*s) {
        game_putchar(*s++, col, row);
        col += 4;
    }
}

static void show_score(void){
    uint16_t hs = data_read_highscore();
    if (score > hs) {
        hs = score;
        data_write_highscore(hs);
    }

    memset(game_fb, 0, LED_COLS * sizeof(uint16_t));

    char buf[10];

    // Top line: "SCORE:XX" at row 0, occupies rows 0-4
    buf[0]='S'; buf[1]='C'; buf[2]='O'; buf[3]='R'; buf[4]='E'; buf[5]=':';
    buf[6] = '0' + (score / 10);
    buf[7] = '0' + (score % 10);
    buf[8] = '\0';
    game_puts(buf, 2, 0);

    // Bottom line: "BEST:XX" at row 6, occupies rows 6-10
    buf[0]='B'; buf[1]='E'; buf[2]='S'; buf[3]='T'; buf[4]=':';
    buf[5] = '0' + (hs / 10);
    buf[6] = '0' + (hs % 10);
    buf[7] = '\0';
    game_puts(buf, 2, 6);

    DelayMs(3000);
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

static void generate_food(void) {
    // Check if there is any free space available
    if (!is_free_space_available()) {
        show_score();
        game_exit();
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

static int move_snake(void)
{
    // commit turn unless it's a 180 reversal
    if (next_dir != (cur_dir + 2) % 4) cur_dir = next_dir;

    Position new_head = snake[0];
    if(cur_dir == DIR_UP) new_head.y--;
    else if(cur_dir == DIR_DOWN) new_head.y++;
    else if(cur_dir == DIR_LEFT) new_head.x--;
    else new_head.x++;

    if (check_collision(new_head)) return 0;

    int ate = (new_head.x == food.x && new_head.y == food.y);
    Position old_tail = snake[snake_length - 1];

    // erase tail before shift, skip only when eating (snake grows)
    if (!ate)
        draw_pixel(snake[snake_length - 1].x, snake[snake_length - 1].y, 0);

    // shift body
    for (int i = snake_length - 1; i > 0; i--)
        snake[i] = snake[i - 1];
    snake[0] = new_head;
    draw_pixel(new_head.x, new_head.y, 1);

    if (ate) {
        score++;

        if (snake_length < SNAKE_MAX_LEN) {
            snake[snake_length] = old_tail; // set before increment
            snake_length++;
        }
        draw_pixel(food.x, food.y, 0);
        generate_food();

        // speed up every 5 food eaten
        if (score % 2 == 0 && speed_ms > SPEED_MIN_MS)
            speed_ms -= SPEED_STEP_MS;
    }

    return 1;
}

static uint16_t game_task(tmosTaskID id, uint16_t events)
{
    if (events & GAME_TICK) {
        if (!move_snake()) {
            show_score();
            game_exit();
        } 
        else {
            draw_pixel(food.x, food.y, 1);
            tmos_start_task(game_taskid, GAME_TICK, MS_TO_TMOS(speed_ms));
        }
        return events ^ GAME_TICK;
    }
    return 0;
}

static void init_game(void) {
    uint16_t year, month, day, hour, minute, second;
    RTC_GetTime(&year, &month, &day, &hour, &minute, &second);
    srand(hour * 3600 + minute * 60 + second);
    snake_length = 1;
    snake[0].x = LED_COLS / 2;
    snake[0].y = LED_ROWS / 2;
    cur_dir = next_dir = DIR_RIGHT;
    score = 0;
    speed_ms = SPEED_INITIAL_MS;
    generate_food();
}

static void game_exit(void)
{
    tmos_stop_task(game_taskid, GAME_TICK);
    return_to_menu();
}

void game_init(void)
{
    game_taskid = TMOS_ProcessEventRegister(game_task);
}

void game_start(uint16_t *fb)
{
    game_fb = fb;
    memset(game_fb, 0, LED_COLS * sizeof(uint16_t));
    init_game();
    draw_pixel(food.x, food.y, 1);

    btn_onOnePress(KEY1, NULL);
    btn_onOnePress(KEY2, NULL);
    auxbtn_onOnePress(KEY3, on_turn_left);
    auxbtn_onOnePress(KEY4, on_turn_right);

    tmos_start_task(game_taskid, GAME_TICK, MS_TO_TMOS(speed_ms));
}