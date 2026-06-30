#include "flappy.h"
#include "leddrv.h"
#include "CH58xBLE_LIB.h"
#include "auxbtn.h"
#include "button.h"
#include "font3x5.h"
#include "CH58x_common.h"
#include <string.h>
#include <stdlib.h>

#define BIRD_COL        5
#define PIPE_WIDTH      2
#define GAP_SIZE        3
#define GAP_MIN         2
#define GAP_MAX         (LED_ROWS - GAP_SIZE - GAP_MIN)   //6
#define PIPE_SPACING    15
#define GRAVITY         2      
#define FLAP_STRENGTH   10    
#define TICK_INIT_MS    120
#define TICK_MIN_MS     50
#define TICK_ACCEL_MS   5      

#define MS_TO_TMOS(ms)  ((uint32_t)(ms) * 1000 / 625)
#define EVT_GAME_TICK   (1 << 0)

// pipes
typedef struct {
    int x;        // left edge - pipe occupies x and x+1
    int gap_top;  
} Pipe;

static tmosTaskID  flappy_taskid = INVALID_TASK_ID;
static uint16_t   *flappy_fb;

static int   bird_y;    
static int   bird_vel;  
static int   score;
static int   tick_ms;
static Pipe  pipes[2];

extern void return_to_menu(void);

static void draw_pixel(int x, int y, int on)
{
    if (x < 0 || x >= LED_COLS || y < 0 || y >= LED_ROWS) return;
    if (on) flappy_fb[x] |=  (1u << y);
    else    flappy_fb[x] &= ~(1u << y);
}

static void draw_pipe(Pipe *p, int on)
{
    for (int col = p->x; col < p->x + PIPE_WIDTH; col++) {
        for (int row = 0; row < LED_ROWS; row++) {
            /* skip the gap rows */
            if (row >= p->gap_top && row < p->gap_top + GAP_SIZE) continue;
            draw_pixel(col, row, on);
        }
    }
}

static void game_putchar(char c, int col, int row)
{
    for (int i = 0; i < 4; i++) {
        if (col + i >= LED_COLS) break;
        flappy_fb[col + i] = (flappy_fb[col + i] & ~(0x1f << row))
                           | (font3x5[c - ' '][i] << row);
    }
}

static void game_puts(const char *s, int col, int row)
{
    while (*s) { game_putchar(*s++, col, row); col += 4; }
}

static int random_gap(void)
{
    return GAP_MIN + (rand() % (GAP_MAX - GAP_MIN + 1));
}

static void flappy_die(void)
{
    tmos_stop_task(flappy_taskid, EVT_GAME_TICK);

    memset(flappy_fb, 0, LED_COLS * sizeof(uint16_t));

    char buf[10];
    game_puts("SCORE:", 2, 0);
    buf[0] = '0' + (score / 10);
    buf[1] = '0' + (score % 10);
    buf[2] = '\0';
    game_puts(buf, 26, 0);

    DelayMs(3000);
    return_to_menu();
}

static void game_tick(void)
{
    int by = bird_y >> 2;
    draw_pixel(BIRD_COL, by, 0);

    // game physics
    bird_vel += GRAVITY;
    bird_y   += bird_vel;
    by = bird_y >> 2;

    // death cuz of walls
    if (by < 0 || by >= LED_ROWS) {
        flappy_die();
        return;
    }

    draw_pixel(BIRD_COL, by, 1);

    for (int i = 0; i < 2; i++) {
        Pipe *p = &pipes[i];

        draw_pipe(p, 0);

        p->x -= 1;

        //pipes going off left edge + increment score
        if (p->x + PIPE_WIDTH <= 0) {
            p->x       = LED_COLS - 1;
            p->gap_top = random_gap();
            score++;
            if (tick_ms > TICK_MIN_MS)
                tick_ms -= TICK_ACCEL_MS;
        }

        draw_pipe(p, 1);

        // collision: bird with pipe
        if (BIRD_COL >= p->x && BIRD_COL < p->x + PIPE_WIDTH) {
            if (by < p->gap_top || by >= p->gap_top + GAP_SIZE) {
                flappy_die();
                return;
            }
        }
    }

    tmos_start_task(flappy_taskid, EVT_GAME_TICK, MS_TO_TMOS(tick_ms));
}

static uint16_t flappy_task(tmosTaskID id, uint16_t events)
{
    if (events & EVT_GAME_TICK) {
        game_tick();
        return events ^ EVT_GAME_TICK;
    }
    return 0;
}

static void on_flap(void)
{
    bird_vel = -FLAP_STRENGTH;
}

void flappy_init(void)
{
    flappy_taskid = TMOS_ProcessEventRegister(flappy_task);
}

void flappy_start(uint16_t *fb)
{
    flappy_fb = fb;
    memset(flappy_fb, 0, LED_COLS * sizeof(uint16_t));

    // seeding
    uint16_t yr, mo, dy, hr, mn, sc;
    RTC_GetTime(&yr, &mo, &dy, &hr, &mn, &sc);
    srand(hr * 3600 + mn * 60 + sc);

    // bird: center of screen, fixed-point - same as actual game (enough to create illusion)
    bird_y   = (LED_ROWS / 2) << 2;
    bird_vel = 0;
    score    = 0;
    tick_ms  = TICK_INIT_MS;

    // pipes: first at col 43, second 15 behind 
    pipes[0].x       = LED_COLS - 1;
    pipes[0].gap_top = random_gap();
    pipes[1].x       = LED_COLS - 1 - PIPE_SPACING;
    pipes[1].gap_top = random_gap();

    draw_pixel(BIRD_COL, LED_ROWS / 2, 1);
    draw_pipe(&pipes[0], 1);
    draw_pipe(&pipes[1], 1);

    // Key 3 is used to play the game
    btn_onOnePress(KEY1, NULL);
    btn_onOnePress(KEY2, NULL);
    btn_onLongPress(KEY1, NULL);
    btn_onLongPress(KEY2, NULL);
    auxbtn_onOnePress(KEY3, on_flap);
    auxbtn_onOnePress(KEY4, NULL);

    tmos_start_task(flappy_taskid, EVT_GAME_TICK, MS_TO_TMOS(tick_ms));
}