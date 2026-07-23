#include "pong.h"
#include "leddrv.h"
#include "CH58xBLE_LIB.h"
#if HW_KEY_COUNT == 4
#include "auxbtn.h"
#endif
#include "button.h"
#include "font3x5.h"
#include "CH58x_common.h"
#include <string.h>
#include <stdlib.h>

static void move_paddle_up(void);
static void move_paddle_down(void);

#define PADDLE_LEN          3       // paddle height in rows
#define BALL_TICK_INIT_MS   120     // starting ball period
#define BALL_TICK_MIN_MS    40      // fastest the ball can go
#define BALL_ACCEL_HITS     4       // speed up every N paddle hits
#define BALL_ACCEL_STEP_MS  8       // ms shaved off per speed-up

#define EVT_BALL_TICK       (1 << 0)
#define EVT_PADDLE_UP       (1 << 1)
#define EVT_PADDLE_DOWN     (1 << 2)
#define PADDLE_REPEAT_MS    80

#define MS_TO_TMOS(ms)  ((uint32_t)(ms) * 1000 / 625)

#define COL_PLAYER_PADDLE   (LED_COLS - 1)  
#define COL_CPU_PADDLE      0                
#define COL_BALL_SPAWN      (LED_COLS / 2)

static tmosTaskID  pong_taskid = INVALID_TASK_ID;
static uint16_t   *pong_fb;

static int ball_x, ball_y;
static int ball_dx, ball_dy;   
static int ball_ms;            
static int hit_count;          //counts paddle hits for accel 

// paddles
static int player_y;           
static int cpu_y;             



extern void return_to_menu(void);

static void draw_pixel(int x, int y, int on)
{
    if (x < 0 || x >= LED_COLS || y < 0 || y >= LED_ROWS) return;
    if (on) pong_fb[x] |=  (1u << y);
    else    pong_fb[x] &= ~(1u << y);
}

static void draw_paddle(int col, int top, int on)
{
    for (int i = 0; i < PADDLE_LEN; i++)
        draw_pixel(col, top + i, on);
}

static void game_putchar(char c, int col, int row)
{
    for (int i = 0; i < 4; i++) {
        if (col + i >= LED_COLS) break;
        pong_fb[col + i] = (pong_fb[col + i] & ~(0x1f << row))
                         | (font3x5[c - ' '][i] << row);
    }
}

static void game_puts(const char *s, int col, int row)
{
    while (*s) { game_putchar(*s++, col, row); col += 4; }
}

static void show_result(uint8_t player_won)
{
    memset(pong_fb, 0, LED_COLS * sizeof(uint16_t));

    if (player_won)
        game_puts("WIN", 6, 0);
    else
        game_puts("LOSE", 6, 0);

    char buf[8];
    buf[0] = 'H'; buf[1] = 'I'; buf[2] = 'T'; buf[3] = 'S'; buf[4] = ':';
    int h = hit_count;
    buf[5] = '0' + (h / 10);
    buf[6] = '0' + (h % 10);
    buf[7] = '\0';
    game_puts(buf, 2, 6);

    DelayMs(3000);
}

// game over
static void pong_exit(uint8_t player_won)
{
    tmos_stop_task(pong_taskid, EVT_BALL_TICK);
    tmos_stop_task(pong_taskid, EVT_PADDLE_UP);
    tmos_stop_task(pong_taskid, EVT_PADDLE_DOWN);
    show_result(player_won);
    return_to_menu();
}

/* CPU AI 
CPU moves at most 1 row per ball tick, and only when the ball is travelling toward it.  
At high ball speeds(which increases as the game goes on) the CPU can't keep up — intentionally beatable.
 */
static void cpu_move(void)
{
    if (ball_dx > 0) return;
    int paddle_center = cpu_y + PADDLE_LEN / 2;

    if (ball_y < paddle_center && cpu_y > 0) {
        draw_paddle(COL_CPU_PADDLE, cpu_y, 0);
        cpu_y--;
        draw_paddle(COL_CPU_PADDLE, cpu_y, 1);
    } 
    else if (ball_y > paddle_center && cpu_y + PADDLE_LEN < LED_ROWS) {
        draw_paddle(COL_CPU_PADDLE, cpu_y, 0);
        cpu_y++;
        draw_paddle(COL_CPU_PADDLE, cpu_y, 1);
    }
}

static void ball_tick(void)
{
    // erase old position
    draw_pixel(ball_x, ball_y, 0);

    int nx = ball_x + ball_dx;
    int ny = ball_y + ball_dy;

    // top/bottom wall bounce
    if (ny < 0){ 
        ny = 0;            
        ball_dy = +1; 
    }
    if (ny >= LED_ROWS){ 
        ny = LED_ROWS - 1; 
        ball_dy = -1; 
    }

    // player paddle (right side)
    if (nx >= COL_PLAYER_PADDLE) {
        if (ny >= player_y && ny < player_y + PADDLE_LEN) {
            nx = COL_PLAYER_PADDLE - 1;
            ball_dx = -1;
            hit_count++;

            // accelerate every N hits
            if (hit_count % BALL_ACCEL_HITS == 0 && ball_ms > BALL_TICK_MIN_MS)
                ball_ms -= BALL_ACCEL_STEP_MS;
        } 
        else {
            // player missed
            draw_pixel(ball_x, ball_y, 0);
            pong_exit(0);
            return;
        }
    }

    // cpu paddle (left side)
    if (nx <= COL_CPU_PADDLE) {
        if (ny >= cpu_y && ny < cpu_y + PADDLE_LEN) {
            nx = COL_CPU_PADDLE + 1;
            ball_dx = +1;
            hit_count++;
        } 
        else {
            // cpu missed — player wins
            draw_pixel(ball_x, ball_y, 0);
            pong_exit(1);
            return;
        }
    }

    ball_x = nx;
    ball_y = ny;

    cpu_move();
    draw_pixel(ball_x, ball_y, 1);
    tmos_start_task(pong_taskid, EVT_BALL_TICK, MS_TO_TMOS(ball_ms));
}

static uint16_t pong_task(tmosTaskID id, uint16_t events)
{
    if (events & EVT_BALL_TICK) {
        ball_tick();
        return events ^ EVT_BALL_TICK;
    }
    if (events & EVT_PADDLE_UP) {
        move_paddle_up();
        if (isPressed(KEY1))
            tmos_start_task(pong_taskid, EVT_PADDLE_UP, MS_TO_TMOS(PADDLE_REPEAT_MS));
        return events ^ EVT_PADDLE_UP;
    }
    if (events & EVT_PADDLE_DOWN) {
        move_paddle_down();
        if (isPressed(KEY2))
            tmos_start_task(pong_taskid, EVT_PADDLE_DOWN, MS_TO_TMOS(PADDLE_REPEAT_MS));
        return events ^ EVT_PADDLE_DOWN;
    }
    return 0;
}

static void move_paddle_up(void)
{
    if (player_y > 0) {
        draw_paddle(COL_PLAYER_PADDLE, player_y, 0);
        player_y--;
        draw_paddle(COL_PLAYER_PADDLE, player_y, 1);
    }
}

static void move_paddle_down(void)
{
    if (player_y + PADDLE_LEN < LED_ROWS) {
        draw_paddle(COL_PLAYER_PADDLE, player_y, 0);
        player_y++;
        draw_paddle(COL_PLAYER_PADDLE, player_y, 1);
    }
}

/*
 One-press: move once immediately.
 Long-press: start the self-rescheduling repeat timer.
 No release event exists in button.c, so the repeat timer runs until the opposing key is pressed (which cancels it).
 */

static void on_key1_press(void)
{
    tmos_stop_task(pong_taskid, EVT_PADDLE_DOWN);
    move_paddle_up();
}

static void on_key2_press(void)
{
    tmos_stop_task(pong_taskid, EVT_PADDLE_UP);
    move_paddle_down();
}

static void on_key1_long(void)
{
    tmos_stop_task(pong_taskid, EVT_PADDLE_DOWN);
    tmos_start_task(pong_taskid, EVT_PADDLE_UP, MS_TO_TMOS(PADDLE_REPEAT_MS));
}

static void on_key2_long(void)
{
    tmos_stop_task(pong_taskid, EVT_PADDLE_UP);
    tmos_start_task(pong_taskid, EVT_PADDLE_DOWN, MS_TO_TMOS(PADDLE_REPEAT_MS));
}

void pong_init(void)
{
    pong_taskid = TMOS_ProcessEventRegister(pong_task);
}

void pong_start(uint16_t *fb)
{
    pong_fb = fb;
    memset(pong_fb, 0, LED_COLS * sizeof(uint16_t));

    // similar to snake 
    uint16_t yr, mo, dy, hr, mn, sc;
    RTC_GetTime(&yr, &mo, &dy, &hr, &mn, &sc);
    srand(hr * 3600 + mn * 60 + sc);

    ball_x  = COL_BALL_SPAWN;
    ball_y  = LED_ROWS / 2;
    ball_dx = (rand() & 1) ? +1 : -1;   // random initial direction
    ball_dy = (rand() & 1) ? +1 : -1;
    ball_ms = BALL_TICK_INIT_MS;
    hit_count = 0;

    player_y = (LED_ROWS - PADDLE_LEN) / 2;
    cpu_y    = (LED_ROWS - PADDLE_LEN) / 2;

    draw_paddle(COL_CPU_PADDLE,    cpu_y,    1);
    draw_paddle(COL_PLAYER_PADDLE, player_y, 1);
    draw_pixel(ball_x, ball_y, 1);

    // KEY1 = up, KEY2 = down
    btn_onOnePress(KEY1, on_key1_press);
    btn_onLongPress(KEY1, on_key1_long);
    btn_onOnePress(KEY2, on_key2_press);
    btn_onLongPress(KEY2, on_key2_long);
#if HW_KEY_COUNT == 4
    auxbtn_onOnePress(KEY3, NULL);
    auxbtn_onOnePress(KEY4, NULL);
#endif

    tmos_start_task(pong_taskid, EVT_BALL_TICK, MS_TO_TMOS(ball_ms));
}