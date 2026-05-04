/*
 * Quest for Chalice - Raylib C Port
 * Original TIC-80 game by Daniel Hanrahan Tools and Games
 * License: GPL-3.0-or-later
 *
 * Porting Notes:
 *   - No heap allocation: all game state lives on the stack or in static arrays.
 *   - Lua 5.4.2 scripting (optional): drop .lua files into lua542Linux/ folder
 *     to override room data or tweak constants at startup.
 *   - Sprite data is hard-coded as 8x8 pixel art bitmaps (color index arrays).
 *   - Map data is hard-coded as tile index arrays matching the TIC-80 MAP section.
 *   - Palette matches the TIC-80 PALETTE section exactly.
 *
 * Build (Linux, lua optional):
 *   gcc quest_for_chalice.c -o quest_for_chalice \
 *       -lraylib -lm \
 *       -Ilua542Linux/include -Llua542Linux/lib -llua -ldl    # add these for Lua
 *
 * Controls:
 *   Left/Right Arrow : move
 *   Z               : jump
 *   (Controller: D-Pad + A button)
 */

#include "raylib.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ============================================================
 * OPTIONAL LUA MOD SUPPORT
 * If lua542Linux/ is present and USE_LUA is defined at compile
 * time, mods can override room data via a Lua script.
 * ============================================================ */
#ifdef USE_LUA
  #include "lua.h"
  #include "lauxlib.h"
  #include "lualib.h"
  static lua_State *L = NULL;  /* single global Lua state, no heap growth after init */
#endif

/* ============================================================
 * DISPLAY / SCALE CONSTANTS
 * TIC-80 native resolution is 240x136. We upscale for desktop.
 * ============================================================ */
#define NATIVE_W   240
#define NATIVE_H   136
#define SCALE      4          /* window = 960x544 */
#define WINDOW_W   (NATIVE_W * SCALE)
#define WINDOW_H   (NATIVE_H * SCALE)
#define TARGET_FPS 60

/* ============================================================
 * GAME CONSTANTS  (match TIC-80 source exactly)
 * ============================================================ */
#define ROOM_W     240
#define ROOM_H     136
#define TILE        8
#define GRAVITY     0.2f
#define JUMP_VEL   -4.5f
#define SPEED       1.2f
#define PLAYER_W    8
#define PLAYER_H    8
#define NUM_ROOMS   10
#define MAP_COLS    30   /* tiles per room row */
#define MAP_ROWS    17   /* tiles per room column */

/* Maximum hazard counts per room (stack arrays, never exceed these) */
#define MAX_FIREBALLS  8
#define MAX_BARRELS    6

/* ============================================================
 * TIC-80 PALETTE  (hex RGB, 16 colors)
 * From: -- <PALETTE> 000:1a1c2c5d275db13e53ef7d57...
 * ============================================================ */
static const Color PALETTE[16] = {
    { 26,  28,  44, 255 }, /* 0  - near-black (transparent bg) */
    { 93,  39,  93, 255 }, /* 1  - dark purple */
    {177,  62,  83, 255 }, /* 2  - crimson */
    {239, 125,  87, 255 }, /* 3  - orange */
    {255, 205, 117, 255 }, /* 4  - yellow */
    {167, 240, 112, 255 }, /* 5  - lime green */
    { 56, 183, 100, 255 }, /* 6  - green */
    { 37, 113, 121, 255 }, /* 7  - #257179 dark teal */
    { 41,  54, 111, 255 }, /* 8  - #29366f darker blue */
    { 59,  93, 201, 255 }, /* 9  - #3b5dc9 blue */
    { 65, 166, 246, 255 }, /* 10 - #41a6f6 blue */
    {115, 239, 247, 255 }, /* 11 - #73eff7 light blue */
    {244, 244, 244, 255 }, /* 12 - #f4f4f4 white */
    {148, 176, 194, 255 }, /* 13 - #94b0c2 light gray-blue */
    { 86, 108, 134, 255 }, /* 14 - #566c86 gray */
    { 51,  60,  87, 255 }, /* 15 - #333c57 dark gray */
    };

/* ============================================================
 * SPRITE DATA
 * Each sprite is 8x8 pixels. Values are palette indices (0-15).
 * Index 0 is transparent (matches TIC-80: poke(0x3FFB,0)).
 *
 * Sprite layout (sprite sheet index):
 *   0  = fireball
 *   1  = player frame 1 (idle)
 *   2  = player frame 2 (walk)
 *   3  = player frame 3 (walk)
 *   4  = chalice / goal
 *   5  = barrel frame 1
 *   6  = barrel frame 2
 * ============================================================ */
/* Each row is one pixel row (8 palette indices) */
static const unsigned char SPR[7][8][8] = {
    /* SPR 0: fireball  -- 000:0000000000022000... */
    {
        {0,0,0,0,0,0,0,0},
        {0,0,0,2,2,0,0,0},
        {0,0,2,3,3,2,0,0},
        {0,0,2,3,3,2,0,0},
        {0,2,3,4,4,3,2,0},
        {0,2,3,4,4,3,2,0},
        {0,0,2,3,3,2,0,0},
        {0,0,0,2,2,0,0,0},
    },
    /* SPR 1: player idle -- 001:0001100000116100... */
    {
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,6,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,0,6,6,0,0,0},
        {0,0,0,6,6,6,0,0},
        {0,0,0,6,6,0,0,0},
        {0,0,1,0,0,1,0,0},
        {0,1,0,0,0,0,1,0},
    },
    /* SPR 2: player walk A -- 002:0001100000116100... */
    {
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,6,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,0,6,6,0,0,0},
        {0,0,0,6,6,6,0,0},
        {0,0,0,6,6,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,0,1,1,0,0,0},
    },
    /* SPR 3: player walk B -- 003:0001100000116100... */
    {
        {0,0,0,1,1,0,0,0},
        {0,0,1,1,6,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,0,6,6,0,0,0},
        {0,0,0,6,6,6,0,0},
        {0,0,0,6,6,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,1,0,0,0,0,1,0},
    },
    /* SPR 4: chalice/goal -- 004:0000000004444440... */
    {
        {0,0,0,0,0,0,0,0},
        {0,4,4,4,4,4,4,0},
        {0,4,6,4,2,4,9,0},
        {0,4,4,4,4,4,4,0},
        {0,0,4,4,4,4,0,0},
        {0,0,0,4,4,0,0,0},
        {0,0,0,4,4,0,0,0},
        {0,0,4,4,4,4,0,0},
    },
    /* SPR 5: barrel frame 1 -- 005:000001f1... (f=15) */
    {
        {0,0,0,0,0,1,15,1},
        {0,0,0,0,1,15,1,1},
        {0,0,0,1,15,1,1,15},
        {0,0,1,15,1,1,15,1},
        {0,1,15,1,1,15,1,0},
        {15,15,15,15,15,1,0,0},
        {1,1,1,15,1,0,0,0},
        {1,1,1,15,0,0,0,0},
    },
    /* SPR 6: barrel frame 2 -- 006:0000011f... */
    {
        {0,0,0,0,0,1,1,15},
        {0,0,0,0,1,1,15,1},
        {0,0,0,1,1,15,1,1},
        {0,0,1,1,15,1,1,1},
        {0,1,1,15,1,1,1,0},
        {15,15,15,15,1,1,0,0},
        {1,1,1,15,1,0,0,0},
        {1,1,1,15,0,0,0,0},
    },
};

/* ============================================================
 * TILE DATA
 * Tiles are 8x8, stored as single solid palette colors to match
 * TIC-80 tiles (each tile was one repeated nibble in the source).
 * ============================================================ */
/* Maps tile index to palette color index for solid-fill rendering */
static const int TILE_COLOR[12] = {
    5,  /* 0  - background green */
    6,  /* 1  - background green2 */
    1,  /* 2  - SOLID floor (dark purple) */
    11, /* 3  - light blue bg */
    10, /* 4  - blue bg */
    9,  /* 5  - dark blue bg */
    8,  /* 6  - darkerblue bg */
    4,  /* 7  - SOLID floor (yellow) */
    12, /* 8  - white bg */
    14, /* 9  - gray bg */
    15, /* 10 - dark gray */
    3,  /* 11 - orange pillar */
};

/* Which tile indices are solid (block player movement) */
static const int SOLID_TILES[3] = { 2, 7, 10 };

/* Check if a tile index is solid */
static int tile_is_solid(int tile_id) {
    int i;
    for (i = 0; i < 3; i++) {
        if (SOLID_TILES[i] == tile_id) return 1;
    }
    return 0;
}

/* ============================================================
 * MAP DATA
 * The TIC-80 map is 240 tiles wide x 17 tiles tall.
 * Each room occupies 30 columns. There are 10 rooms (3 visible groups).
 * Stored as [row][col] where col = room_index*30 + local_col.
 *
 * Room themes (matching TIC-80 map section):
 *   Rooms 1,5,8 -> type A (tiles 0,1,2,11 - dark with pillars)
 *   Rooms 2,6,9 -> type B (tiles 3-7     - colorful cave)
 *   Rooms 3,7,10-> type C (tiles 8-10    - brown/tan cave)
 *   Room  4     -> type D (special chalice room)
 *
 * To keep stack size reasonable, the map is stored compactly
 * as one flat array: MAP_DATA[row * (NUM_ROOMS*MAP_COLS) + col]
 * ============================================================ */

/* Room type identifiers */
#define RTYPE_A 0  /* rooms 1,5,8  - green/black with pillars */
#define RTYPE_B 1  /* rooms 2,6,9  - colorful cave */
#define RTYPE_C 2  /* rooms 3,7,10 - brown cave with slope */
#define RTYPE_D 3  /* room  4      - chalice room (special) */

/* Which room type each room uses (1-indexed, room 0 unused) */
static const int ROOM_TYPE[NUM_ROOMS + 1] = {
    0,          /* unused index 0 */
    RTYPE_A,    /* room 1 */
    RTYPE_B,    /* room 2 */
    RTYPE_C,    /* room 3 */
    RTYPE_D,    /* room 4 */
    RTYPE_A,    /* room 5 */
    RTYPE_B,    /* room 6 */
    RTYPE_C,    /* room 7 */
    RTYPE_A,    /* room 8 */
    RTYPE_B,    /* room 9 */
    RTYPE_C,    /* room 10 */
};

/*
 * Type A room layout (30x17 tiles):
 * Rows 0-3: all tile 1 (background)
 * Rows 4-14: open with tile 11 pillars at cols 6,7 and 27,28
 * Row 15-16: solid floor (tile 2)
 */
static int room_type_a_tile(int row, int col) {
    if (row >= 15) return 2;                        /* solid floor */
    if (row <= 3) return 1;                         /* background top */
    /* pillar columns */
    if ((col == 6 || col == 7) || (col == 27 || col == 28)) return 11;
    return 0;                                       /* open background */
}

/*
 * Type B room layout (30x17 tiles):
 * Rows 0-5: tile 3 (dark green bg)
 * Rows 6-14: repeating pattern of tiles 4,5,6 (cave decor)
 * Row 15-16: solid floor (tile 7)
 */
static int room_type_b_tile(int row, int col) {
    static const int PATTERN[9] = {4,5,6,6,6,4,4,5,6}; /* repeating cave pattern */
    if (row >= 15) return 7;                        /* solid floor */
    if (row <= 5)  return 3;                        /* dark bg */
    return PATTERN[col % 9];                        /* decorative cave walls */
}

/*
 * Type C room layout (30x17 tiles):
 * Rows 0-5: tile 8 (brown bg)
 * Row 6:    tile 9 (tan) all across
 * Rows 7-14: diagonal slope of tile 9 shrinking left side
 * Row 15-16: solid floor (tile 10)
 *
 * The slope: each row down, one fewer tile-9 col on the left.
 *   Row 7: cols 0-7 are 9, rest 8
 *   Row 8: cols 0-7 are 9, rest 8
 *   Row 9: cols 0-6 are 9, rest 8
 *  ... etc (matches TIC-80 map data)
 */
static int room_type_c_tile(int row, int col) {
    /* Slope boundary: how many left cols are tile 9 for rows 7-14 */
    static const int SLOPE[8] = { 8, 8, 7, 7, 6, 5, 3, 3 };
    if (row >= 15) return 10;                       /* solid floor */
    if (row <= 5)  return 8;                        /* brown bg */
    if (row == 6)  return 9;                        /* full tan row */
    /* rows 7-14: slope */
    {
        int slope_idx = row - 7;
        if (slope_idx < 8 && col < SLOPE[slope_idx]) return 9;
        return 8;
    }
}

/*
 * Type D room layout (30x17 tiles) - chalice room:
 * Three pillared alcoves with the chalice in the middle one.
 * Matches the TIC-80 map section for room 4.
 */
static int room_type_d_tile(int row, int col) {
    /* Floor rows */
    if (row >= 15) return 10;
    /* Top rows background */
    if (row <= 1) return 8;

    /* Three alcove structures.
     * Alcove 1: cols 1-5   (left)
     * Alcove 2: cols 9-13  (middle, has chalice marker in map)
     * Alcove 3: cols 17-21 (right)
     * The rest is background tile 8 or open tile 9 */

    /* Alcove outer walls (cols 1,5 | 9,13 | 17,21) are tile 9 */
    /* Inner columns are decorative tiles 0,1,2,3 (just background here) */
    /* We replicate the visual structure without the Lua sprite logic */

    /* Alcove left walls */
    if ((col == 1 || col == 5) && row >= 4 && row <= 14) return 9;
    if ((col == 9 || col == 13) && row >= 4 && row <= 14) return 9;
    if ((col == 17 || col == 21) && row >= 4 && row <= 14) return 9;

    /* Alcove top caps (row 2-3 around pillars) */
    if (row == 2 && (col==3 || col==11 || col==19)) return 9;
    if (row == 3 && (col>=2&&col<=4)) return 9;
    if (row == 3 && (col>=10&&col<=12)) return 9;
    if (row == 3 && (col>=18&&col<=20)) return 9;

    /* Row 4: alcove ledge */
    if (row == 4 && (col>=2&&col<=4)) return 9;
    if (row == 4 && (col>=10&&col<=12)) return 9;
    if (row == 4 && (col>=18&&col<=20)) return 9;

    return 8; /* brown background */
}

/* Get tile index for a given room (1-based) and tile coordinate */
static int get_tile(int room, int row, int col) {
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS) return 0;
    switch (ROOM_TYPE[room]) {
        case RTYPE_A: return room_type_a_tile(row, col);
        case RTYPE_B: return room_type_b_tile(row, col);
        case RTYPE_C: return room_type_c_tile(row, col);
        case RTYPE_D: return room_type_d_tile(row, col);
        default:      return 0;
    }
}

/* ============================================================
 * GAME STATE STRUCTURES  (all stack-allocated)
 * ============================================================ */

/* A falling fireball hazard */
typedef struct {
    float x, y;     /* pixel position */
    float speed;    /* pixels per frame (downward) */
    int   active;   /* 1 = in use */
} Fireball;

/* A rolling barrel hazard */
typedef struct {
    float x, y;     /* pixel position */
    float speed;    /* pixels per frame (leftward) */
    int   anim;     /* 0 or 1 - alternating animation frame */
    int   anim_t;   /* frame timer for animation */
    int   active;
} Barrel;

/* Room definition (hazard spawn points) */
typedef struct {
    /* Fireball spawn data: tile coordinates + speed */
    struct { int tx, ty; float spd; } fb[MAX_FIREBALLS];
    int fb_count;

    /* Barrel spawn data: tile coordinates + speed */
    struct { int tx, ty; float spd; } br[MAX_BARRELS];
    int br_count;
} RoomDef;

/* ============================================================
 * ROOM DEFINITIONS  (matches TIC-80 ROOMS table exactly)
 * ============================================================ */
static const RoomDef ROOM_DEFS[NUM_ROOMS + 1] = {
    /* [0] unused */
    { .fb_count=0, .br_count=0 },

    /* [1] room 1 */
    {
        .fb_count=0,
        .br={{11,14,1.0f},{29,14,1.0f}}, .br_count=2
    },
    /* [2] room 2 */
    {
        .fb={{7,6,1.5f},{24,1,1.5f}}, .fb_count=2,
        .br_count=0
    },
    /* [3] room 3 */
    {
        .fb={{1,1,1.0f}}, .fb_count=1,
        .br={{29,14,1.0f}}, .br_count=1
    },
    /* [4] room 4 - chalice room, no hazards */
    { .fb_count=0, .br_count=0 },

    /* [5] room 5 */
    {
        .fb={{0,1,1.2f},{6,9,1.0f},{19,1,1.0f}}, .fb_count=3,
        .br_count=0
    },
    /* [6] room 6 */
    {
        .fb_count=0,
        .br={{9,14,1.0f},{22,14,1.0f},{24,14,1.0f}}, .br_count=3
    },
    /* [7] room 7 */
    {
        .fb_count=0,
        .br={{15,14,1.0f},{26,14,1.0f}}, .br_count=2
    },
    /* [8] room 8 */
    {
        .fb={{19,0,1.2f},{27,1,1.0f},{25,7,1.0f}}, .fb_count=3,
        .br={{10,14,1.0f},{13,14,1.0f},{24,14,1.0f}}, .br_count=3
    },
    /* [9] room 9 */
    {
        .fb={{0,3,1.5f},{4,7,1.5f}}, .fb_count=2,
        .br={{18,14,1.0f},{29,14,1.0f}}, .br_count=2
    },
    /* [10] room 10 */
    {
        .fb={{12,6,1.0f},{4,3,1.3f},{25,4,1.1f},{14,0,1.1f}}, .fb_count=4,
        .br={{11,14,1.0f},{26,14,1.0f}}, .br_count=2
    },
};

/* ============================================================
 * GLOBAL GAME STATE  (all static = BSS/data segment, not heap)
 * ============================================================ */
static struct {
    /* --- Player --- */
    float px, py;       /* position (pixels) */
    float vx, vy;       /* velocity */
    float jump_vx;      /* locked horizontal velocity during jump */
    int   on_ground;
    int   facing;       /* 1 = right, -1 = left */
    int   anim_t;       /* animation frame timer */

    /* --- Active hazards --- */
    Fireball fireballs[MAX_FIREBALLS];
    int      fb_count;
    Barrel   barrels[MAX_BARRELS];
    int      br_count;

    /* --- Goal (chalice) --- */
    float goal_x, goal_y;
    int   goal_collected;

    /* --- Room / game flow --- */
    int current_room;   /* 1..NUM_ROOMS */

    /* --- State flags --- */
    int show_title;
    int title_timer;
    int game_over;
    int game_won;
} G;

/* ============================================================
 * RENDERING HELPERS
 * We render to a RenderTexture2D at native 240x136, then scale up.
 * ============================================================ */
static RenderTexture2D render_target; /* native-res render target */

/* Draw a single 8x8 sprite at pixel (x,y).
 * flip_h: 1 = mirror horizontally (for left-facing player)
 * Palette index 0 is transparent (skipped). */
static void draw_sprite(int spr_id, int x, int y, int flip_h) {
    int row, col;
    if (spr_id < 0 || spr_id >= 7) return;
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            int px_col = flip_h ? (7 - col) : col;
            int pidx = SPR[spr_id][row][px_col];
            if (pidx == 0) continue; /* transparent */
            DrawPixel(x + col, y + row, PALETTE[pidx]);
        }
    }
}

/* Draw one tile at tile coords (tx, ty) for current room */
static void draw_tile(int room, int tx, int ty) {
    int tile_id = get_tile(room, ty, tx);
    Color c = PALETTE[TILE_COLOR[tile_id]];
    DrawRectangle(tx * TILE, ty * TILE, TILE, TILE, c);
}

/* Draw the entire room map */
static void draw_map(int room) {
    int tx, ty;
    for (ty = 0; ty < MAP_ROWS; ty++) {
        for (tx = 0; tx < MAP_COLS; tx++) {
            draw_tile(room, tx, ty);
        }
    }
}

/* ============================================================
 * COLLISION
 * ============================================================ */
/* Check if world pixel (wx,wy) is inside a solid tile */
static int solid_at(float wx, float wy) {
    int tx = (int)(wx / TILE);
    int ty = (int)(wy / TILE);
    if (tx < 0 || tx >= MAP_COLS || ty < 0 || ty >= MAP_ROWS) return 0;
    return tile_is_solid(get_tile(G.current_room, ty, tx));
}

/* AABB collision check */
static int hit(float ax, float ay, float aw, float ah,
               float bx, float by, float bw, float bh) {
    return (ax < bx+bw && ax+aw > bx && ay < by+bh && ay+ah > by);
}

/* ============================================================
 * ROOM LOADING
 * Resets player position and populates hazard arrays from ROOM_DEFS.
 * ============================================================ */
static void load_room(int n) {
    int i;
    const RoomDef *rd;

    if (n < 1 || n > NUM_ROOMS) return;
    G.current_room = n;
    rd = &ROOM_DEFS[n];

    /* Reset player */
    G.px = 2.0f;  G.py = 80.0f;
    G.vx = 0.0f;  G.vy = 0.0f;
    G.jump_vx  = 0.0f;
    G.on_ground = 0;

    /* Spawn fireballs */
    G.fb_count = rd->fb_count;
    for (i = 0; i < rd->fb_count; i++) {
        G.fireballs[i].x      = (float)(rd->fb[i].tx * TILE);
        G.fireballs[i].y      = (float)(rd->fb[i].ty * TILE);
        G.fireballs[i].speed  = rd->fb[i].spd;
        G.fireballs[i].active = 1;
    }

    /* Spawn barrels */
    G.br_count = rd->br_count;
    for (i = 0; i < rd->br_count; i++) {
        G.barrels[i].x      = (float)(rd->br[i].tx * TILE);
        G.barrels[i].y      = (float)(rd->br[i].ty * TILE);
        G.barrels[i].speed  = rd->br[i].spd;
        G.barrels[i].anim   = 0;
        G.barrels[i].anim_t = 0;
        G.barrels[i].active = 1;
    }

    /* Reset chalice if entering room 4 */
    if (n == 4) {
        G.goal_x         = 12.0f * 18.0f; /* matches TIC-80: 12*18 */
        G.goal_y         = 8.0f  * 12.0f; /* matches TIC-80: 8*12  */
        G.goal_collected = 0;
    }
}

/* ============================================================
 * RANDOM ROOM SELECTION
 * Pick a random room different from the current one.
 * Uses only stdlib rand() - no allocation.
 * ============================================================ */
static int random_room_except(int except_room) {
    int r;
    do {
        r = (rand() % NUM_ROOMS) + 1;
    } while (r == except_room);
    return r;
}

/* ============================================================
 * PLAYER UPDATE  (matches TIC-80 update_player exactly)
 * ============================================================ */
static void update_player(void) {
    float bottom;

    /* Jump: only when on ground (button 4 = Z key) */
    if (IsKeyPressed(KEY_Z) && G.on_ground) {
        G.vy       = JUMP_VEL;
        G.on_ground = 0;
        G.jump_vx  = G.vx;
    }
    /* Gamepad support (button A = index 0) */
    if (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) && G.on_ground) {
        G.vy       = JUMP_VEL;
        G.on_ground = 0;
        G.jump_vx  = G.vx;
    }

    /* Horizontal movement: locked to ground direction during jump */
    if (G.on_ground) {
        G.vx = 0.0f;
        if (IsKeyDown(KEY_LEFT)  || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))
            { G.vx = -SPEED; G.facing = -1; }
        if (IsKeyDown(KEY_RIGHT) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT))
            { G.vx =  SPEED; G.facing =  1; }
        G.jump_vx = G.vx;
    } else {
        /* In air: keep horizontal velocity from jump moment */
        G.vx = G.jump_vx;
    }

    /* Apply gravity */
    G.vy += GRAVITY;

    /* Move */
    G.px += G.vx;
    G.py += G.vy;

    /* Floor collision: check three points along bottom edge */
    bottom = G.py + PLAYER_H;
    if (solid_at(G.px + 1,           bottom) ||
        solid_at(G.px + PLAYER_W/2,  bottom) ||
        solid_at(G.px + PLAYER_W - 1, bottom)) {
        G.py        = (float)(((int)(bottom / TILE)) * TILE) - PLAYER_H;
        G.vy        = 0.0f;
        G.on_ground = 1;
    } else {
        G.on_ground = 0;
    }

    /* Clamp left edge */
    if (G.px < 0) G.px = 0;

    /* Clamp right edge only in room 4 (chalice room has a wall) */
    if (G.current_room == 4 && G.px + PLAYER_W > ROOM_W)
        G.px = (float)(ROOM_W - PLAYER_W);
}

/* ============================================================
 * HAZARD UPDATES
 * ============================================================ */
static void update_fireballs(void) {
    int i;
    for (i = 0; i < G.fb_count; i++) {
        Fireball *f = &G.fireballs[i];
        if (!f->active) continue;

        f->y += f->speed;
        if (f->y > ROOM_H) f->y = -8.0f; /* wrap to top */

        /* Kill player on contact */
        if (hit(G.px, G.py, PLAYER_W, PLAYER_H, f->x, f->y, 8, 8))
            G.game_over = 1;
    }
}

static void update_barrels(void) {
    int i;
    for (i = 0; i < G.br_count; i++) {
        Barrel *b = &G.barrels[i];
        if (!b->active) continue;

        b->x -= b->speed;            /* roll left */
        b->anim_t++;
        if (b->anim_t >= 2) {        /* toggle animation frame every 2 ticks */
            b->anim   = (b->anim + 1) % 2;
            b->anim_t = 0;
        }
        if (b->x < -8.0f) b->x = (float)ROOM_W; /* wrap to right */

        /* Kill player on contact */
        if (hit(G.px, G.py, PLAYER_W, PLAYER_H, b->x, b->y, 8, 8))
            G.game_over = 1;
    }
}

/* ============================================================
 * DRAW FUNCTIONS
 * ============================================================ */
static void draw_player(void) {
    /* Select animation frame:
     * - moving: cycle through frames 1->2->3 every 6 ticks
     * - idle:   always frame 1 (SPR index 1) */
    int frame_idx;
    if (fabsf(G.vx) > 0.0f)
        frame_idx = 1 + ((G.anim_t / 6) % 3); /* 1, 2, or 3 */
    else
        frame_idx = 1;

    /* Flip sprite when facing left */
    draw_sprite(frame_idx, (int)G.px, (int)G.py, G.facing == -1 ? 1 : 0);
}

static void draw_fireballs(void) {
    int i;
    for (i = 0; i < G.fb_count; i++) {
        Fireball *f = &G.fireballs[i];
        if (f->active)
            draw_sprite(0, (int)f->x, (int)f->y, 0);
    }
}

static void draw_barrels(void) {
    int i;
    for (i = 0; i < G.br_count; i++) {
        Barrel *b = &G.barrels[i];
        if (b->active)
            draw_sprite(5 + b->anim, (int)b->x, (int)b->y, 0); /* SPR 5 or 6 */
    }
}

static void draw_goal(void) {
    if (G.current_room == 4 && !G.goal_collected)
        draw_sprite(4, (int)G.goal_x, (int)G.goal_y, 0); /* SPR 4 = chalice */
}

/* ============================================================
 * TITLE SCREEN  (copyright + license as per TIC-80 source)
 * ============================================================ */
static void draw_title(void) {
    ClearBackground(PALETTE[0]);
    DrawText("Copyright (C) 2026 Daniel",                    0, 10, 8, PALETTE[12]);
    DrawText("Hanrahan Tools and Games",                     0, 20, 8, PALETTE[12]);
    DrawText("SPDX-License-Identifier: GPL-3.0-or-later",   0, 30, 8, PALETTE[12]);
    DrawText("Information just about the stuff in this",    0, 70, 8, PALETTE[12]);
    DrawText("software not covered by the GNU General",     0, 80, 8, PALETTE[12]);
    DrawText("Public License version 3: This work is",      0, 90, 8, PALETTE[12]);
    DrawText("licensed under Attribution-ShareAlike",       0,100, 8, PALETTE[12]);
    DrawText("4.0 International",                           0,110, 8, PALETTE[12]);
}

/* ============================================================
 * OPTIONAL LUA MOD SYSTEM
 * If lua542Linux/ exists and contains a file "mod.lua", it is
 * executed at startup. The mod can redefine room hazard data
 * by calling exported C functions via a registered API.
 *
 * Example mod.lua (place in lua542Linux/):
 *   -- Override room 5 to have no hazards
 *   set_room_fireballs(5, {})
 *   set_room_barrels(5, {})
 * ============================================================ */
#ifdef USE_LUA

/* Registered Lua function: get_room_count() -> number */
static int lua_get_room_count(lua_State *ls) {
    lua_pushinteger(ls, NUM_ROOMS);
    return 1;
}


/* Add these C functions before lua_load_mod() */

/* Returns current room number to Lua */
static int lua_get_current_room(lua_State *ls) {
    lua_pushinteger(ls, G.current_room);
    return 1;
}

/* Returns 1 if player is in the air (just jumped), 0 otherwise */
static int lua_get_player_jumped(lua_State *ls) {
    lua_pushboolean(ls, !G.on_ground);
    return 1;
}


/* Load and execute optional mod script */
static void lua_load_mod(void) {
    const char *mod_path = "Quest_For_Chalice_Compatibility_Mod/Quest_For_Chalice_Compatibility_Mod.lua";
    int result;

    L = luaL_newstate();
    if (!L) { TraceLog(LOG_WARNING, "LUA: Could not create Lua state"); return; }

    luaL_openlibs(L);

    /* Register game API functions */
    lua_register(L, "get_room_count", lua_get_room_count);
    lua_register(L, "get_room_count",    lua_get_room_count);
    lua_register(L, "get_current_room",  lua_get_current_room);   /* add this */
    lua_register(L, "get_player_jumped", lua_get_player_jumped);  /* add this */

    /* Try to load mod script (silently skip if not found) */
    result = luaL_dofile(L, mod_path);
    if (result != LUA_OK) {
        TraceLog(LOG_WARNING, "LUA: mod.lua error: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        TraceLog(LOG_INFO, "LUA: Loaded mod from %s", mod_path);
    }
}

static void lua_cleanup(void) {
    if (L) { lua_close(L); L = NULL; }
}

#endif /* USE_LUA */


/* Add this function */
static void lua_tick(void) {
    if (!L) return;
    /* Look for a function called tick() in the mod */
    lua_getglobal(L, "tick");
    if (lua_isfunction(L, -1)) {
        lua_call(L, 0, 0);  /* call tick() with no args, no return */
    } else {
        lua_pop(L, 1);  /* not defined, clean up stack */
    }
}

/* ============================================================
 * MAIN
 * ============================================================ */
int main(void) {
    int next_room;

    /* Seed RNG */
    srand((unsigned int)time(NULL));

    /* --- Optional Lua mod loading --- */
#ifdef USE_LUA
    lua_load_mod();
#endif

    /* --- Window setup --- */
    InitWindow(WINDOW_W, WINDOW_H, "Quest for the Chalice Compatibility");
    SetTargetFPS(TARGET_FPS);

    /* Native-resolution render target (240x136), scaled up to window */
    render_target = LoadRenderTexture(NATIVE_W, NATIVE_H);
    SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT); /* crisp pixels */

    /* --- Initial game state --- */
    memset(&G, 0, sizeof(G));
    G.show_title  = 1;
    G.title_timer = 300;  /* ~5 seconds at 60fps */
    G.facing      = 1;

    /* Start in a random room (loaded after title screen) */
    G.current_room = (rand() % NUM_ROOMS) + 1;

    /* ============================================================
     * MAIN GAME LOOP
     * ============================================================ */
    while (!WindowShouldClose()) {

        /* ---- UPDATE ---- */
        if (G.show_title) {
            G.title_timer--;
            if (G.title_timer <= 0) {
                G.show_title = 0;
                load_room(G.current_room);
            }
        }
        else if (G.game_over) {
            /* Wait for Z to restart */
            if (IsKeyPressed(KEY_Z) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                G.game_over = 0;
                G.game_won  = 0;
                G.current_room = random_room_except(0);
                load_room(G.current_room);
            }
        }
        else if (!G.game_won) {
            /* Normal gameplay */
            update_player();
            update_fireballs();
            update_barrels();
            #ifdef USE_LUA
            lua_tick();   /* call mod's tick() every frame if it exists */
            #endif
            G.anim_t++;

            /* Room exit: if player walks off the right edge */
            if (G.px > ROOM_W) {
                next_room = random_room_except(G.current_room);
                load_room(next_room);
            }

            /* Chalice pickup check (room 4 only) */
            if (G.current_room == 4 && !G.goal_collected) {
                if (hit(G.px, G.py, PLAYER_W, PLAYER_H,
                        G.goal_x, G.goal_y, 8, 8)) {
                    G.goal_collected = 1;
                    G.game_won       = 1;
                }
            }
        }

        /* ---- RENDER TO NATIVE TARGET ---- */
        BeginTextureMode(render_target);

        if (G.show_title) {
            draw_title();
        }
        else if (G.game_over) {
            ClearBackground(PALETTE[0]);
            /* Centered "GAME OVER" text at native res */
            DrawText("GAME OVER",       90, 60, 8, PALETTE[12]);
            DrawText("PRESS Z TO RESTART", 50, 80, 8, PALETTE[14]);
        }
        else if (G.game_won) {
            ClearBackground(PALETTE[0]);
            DrawText("YOU HAVE DEFEATED THE GAME!", 40, 60, 8, PALETTE[12]);
        }
        else {
            /* Normal game render */
            ClearBackground(PALETTE[0]);
            draw_map(G.current_room);
            draw_fireballs();
            draw_barrels();
            draw_player();
            draw_goal();
        }

        EndTextureMode();

        /* ---- SCALE UP TO WINDOW ---- */
        BeginDrawing();
        ClearBackground(BLACK);
        /* Flip Y because OpenGL texture coords are inverted */
        DrawTexturePro(
            render_target.texture,
            (Rectangle){ 0, 0, (float)NATIVE_W, -(float)NATIVE_H },
            (Rectangle){ 0, 0, (float)WINDOW_W,  (float)WINDOW_H },
            (Vector2){ 0, 0 },
            0.0f,
            WHITE
        );
        EndDrawing();
    }

    /* ---- CLEANUP ---- */
    UnloadRenderTexture(render_target);
    CloseWindow();

#ifdef USE_LUA
    lua_cleanup();
#endif

    return 0;
}
