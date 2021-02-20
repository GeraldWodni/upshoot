#include <gb/gb.h>
#include <stdio.h>
#include <rand.h>
#include "media/tiles.c"

#define NUMBER_OF_TILES 20
#define TILE_EMPTY 0
#define TILE_PLAYER 1
#define TILE_ROCKET 2
#define TILE_ENEMY 3
#define TILE_RAY 4
#define TILE_EXPLOSION 5
#define TILE_ZERO 10

#define REPEAT_FRAMES 4
#define EXPLOSION_FRAMES 42

// 160x144 px = 20x18 tiles
#define TW 20
#define TH 18
#define ENEMIES TH-1
#define ENEMY_MIN_SPEED 40
#define ENEMY_MAX_SPEED 1
#define RAY_DURATION 8

#define HIGHSCORE_X TW-1
#define HIGHSCORE_Y TH-1

INT8 gameRunning = -1;
INT8 enemies[ENEMIES];
INT8 explosions_x[ENEMIES];
INT8 explosions_time[ENEMIES];
INT8 player;
INT8 shot;
INT8 enemySpeed = ENEMY_MIN_SPEED;

INT16 highscore = 0;

void setTile( INT8 x, INT8 y, INT8 tile ) {
    unsigned char buffer[1] = { tile };
    set_bkg_tiles( x, y, 1, 1, buffer );
}

void movePlayer( INT8 direction ) {
    INT8 lastPlayer = player;
    player += direction;
    if( player < 0 )
        player = 0;
    else if( player >= ENEMIES )
        player = ENEMIES -1;

    setTile( 0, lastPlayer, TILE_EMPTY );
    setTile( 0, player, TILE_PLAYER );
}

void updateHighscore() {
    INT8 x = HIGHSCORE_X;
    INT16 score = highscore;
    while( score > 0 ) {
        INT8 remainder = score % 10;
        score-=remainder;
        score/=10;
        setTile( x--, HIGHSCORE_Y, TILE_ZERO+remainder );
    }
}

void updateExplosions() {
    for( UINT8 i = 0; i < ENEMIES; i++ )
        if( explosions_time[i] > 0 ) {
            explosions_time[i]--;
            if( explosions_time[i] == 0 )
                setTile( explosions_x[i], i, TILE_EMPTY );
            else
                setTile( explosions_x[i], i, TILE_EXPLOSION );
        }
}

INT8 enemyRepeat = 0;
void updateEnemies() {
    if( enemyRepeat-- > 0 )
        return;
    enemyRepeat = enemySpeed;

    for( UINT8 i = 0; i < ENEMIES; i++ ) {
        if( enemies[i] < TW ) {
            setTile( enemies[i], i, TILE_EMPTY );
            enemies[i]--;
            setTile( enemies[i], i, TILE_ENEMY );
        }
        else
            enemies[i]--;

        if( enemies[i] == 0 )
            gameRunning = 0;
    }
}

INT8 randomEnemyX() {
    return (rand() & 0x1F) + TW + RAY_DURATION;
}

INT8 shootRepeat = 0;
void shoot() {
    /* check for enemy collisions */
    if( shootRepeat >= 2 ) {
        if( enemies[shot] < TW ) {
            NR44_REG = 0xC0; // TL-- ---- Trigger, Length enable

            highscore++;

            explosions_x[shot] = enemies[shot];
            explosions_time[shot] = EXPLOSION_FRAMES;

            enemies[shot]=randomEnemyX();
        }
    }

    /* sound */
    if( shootRepeat == RAY_DURATION ) {
        NR14_REG = 0x87; // TL-- -FFF Trigger, Length enable, Frequency MSB
        //NR44_REG = 0x80; // NR44 FF23 TL-- ---- Trigger, Length enable
    }

    /* draw enemies */
    if( shootRepeat > 2 ) {
        shootRepeat--;
        for( UINT8 x = 1; x < TW; x++ )
            setTile( x, shot, TILE_RAY );
        return;
    }
    if( shootRepeat == 2 ) {
        shootRepeat--;
        for( UINT8 x = 1; x < TW; x++ )
            setTile( x, shot, TILE_EMPTY );
        return;
    }
    if( shootRepeat == 1 && joypad() != J_A )
        shootRepeat = 0;
}

void init() {
    set_bkg_data( 0, NUMBER_OF_TILES, Tiles );
    fill_bkg_rect( 0, 0, TW, TH, 0 );

    for( UINT8 i = 0; i < ENEMIES; i++ ) {
        enemies[i] = randomEnemyX();
        explosions_time[i] = 0;
    }

    player = TH/2;
    movePlayer( 0 );

    gameRunning = -1;

    // sound registers: https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
    NR52_REG = 0x80; // enable sound
    NR51_REG = 0xFF; // left, right enable
    NR50_REG = 0x77; // left, right max volume

    // square 1
    NR10_REG = 0x1E; // -PPP NSSS Sweep period, negate, shift
    NR11_REG = 0x10; // DDLL LLLL Duty, Length load (64-L)
    NR12_REG = 0xF3; // VVVV APPP Starting volume, Envelope add mode, period
    NR13_REG = 0xFF; // FFFF FFFF Frequency LSB

    // square 2
    NR21_REG = 0x10; // DDLL LLLL Duty, Length load (64-L)
    NR22_REG = 0xF0; // VVVV APPP Starting volume, Envelope add mode, period
    NR23_REG = 0xFF; // FFFF FFFF Frequency LSB

    // noise
    NR41_REG = 0x03; // --LL LLLL Length load (64-L)
    NR42_REG = 0xF7; // VVVV APPP Starting volume, Envelope add mode, period
    NR43_REG = 0x3F; // SSSS WDDD Clock shift, Width mode of LFSR, Divisor code
    //NR44_REG = 0xC0; // TL-- ---- Trigger, Length enable

    //INT8 wait = -1;
    //while(wait) {
    //    switch( joypad() ) {
    //        case J_UP:
    //            NR14_REG = 0x87;
    //            break;
    //        case J_DOWN:
    //            NR44_REG = 0xC0; // TL-- ---- Trigger, Length enable
    //            break;
    //        case J_RIGHT:
    //            NR24_REG = 0xC1; // NR24 FF19 TL-- -FFF Trigger, Length enable, Frequency MSB
    //            break;
    //        case J_A:
    //            wait = 0;
    //            break;
    //    }
    //}
}

void main() {
    UINT8 repeat = 0;

    SHOW_BKG;
    DISPLAY_ON;

    while(1) {
        init();

        highscore = 0;

        while(gameRunning) {
            updateEnemies();
            updateExplosions();
            shoot();
            updateHighscore();

            switch( joypad() ) {
                case J_UP:
                    if( repeat-- == 0 ) {
                        movePlayer(-1);
                        repeat = REPEAT_FRAMES;
                    }
                    break;
                case J_DOWN:
                    if( repeat-- == 0 ) {
                        movePlayer(1);
                        repeat = REPEAT_FRAMES;
                    }
                    break;
                case J_A:
                    if( shootRepeat == 0 ) {
                        shot = player;
                        shootRepeat = RAY_DURATION;
                    }
                    break;
                default:
                    repeat = 0;
            }
            wait_vbl_done();
        }

        setTile( 0, 0, TILE_EMPTY );
        printf("\n:;\n\n\n\n\nScore: %d\nGame Over, sorry :(\n\nHave a nice day ;)\n\nPress B to restart\n\n\n\n\n\n", highscore);
        while( joypad() != J_B ) wait_vbl_done;
    }
}
