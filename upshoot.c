#include <gb/gb.h>
#include <stdio.h>
#include <rand.h>
#include "media/tiles.c"

#define NUMBER_OF_TILES 33
#define TILE_EMPTY 0
#define TILE_PLAYER 1
#define TILE_ROCKET 2
#define TILE_ENEMY 3
#define TILE_RAY 4
#define TILE_EXPLOSION 5
#define TILE_GRADIENT 6
#define TILE_ZERO 10
#define TILE_SCORE 20
#define TILE_SCORE_W 3
#define TILE_GAME_OVER 23
#define TILE_GAME_OVER_W 6
#define TILE_PRESS_B 29
#define TILE_PRESS_B_W 4

#define SPRITE_ROCKET 0
#define SPRITE_PLAYER 1
#define SPRITE_ENEMIES 2
#define SPRITE(I) shadow_OAM[I]
#define ENEMY_SPRITE(I) SPRITE(SPRITE_ENEMIES+I)

#define REPEAT_FRAMES 4
#define EXPLOSION_FRAMES 42

#define TARGET_BKG 0
#define TARGET_WIN 1

// 160x144 px = 20x18 tiles
#define PW 160
#define PH 144
#define TW 20
#define TH 18
#define ENEMIES TH-1
#define ENEMY_MIN_SPEED 8
#define ENEMY_MAX_SPEED 1
#define RAY_DURATION 8

#define HIGHSCORE_X TW-1
#define HIGHSCORE_Y 0

INT8 gameRunning = -1;
INT8 explosions_time[ENEMIES];
INT8 player;
INT8 shot;
UINT8 enemySpeed = ENEMY_MIN_SPEED;

INT16 highscore = 0;

INT8 tileTarget = TARGET_BKG;
void setTile( INT8 x, INT8 y, INT8 tile ) {
    unsigned char buffer[1] = { tile };
    if( tileTarget == TARGET_BKG )
        set_bkg_tiles( x, y, 1, 1, buffer );
    else
        set_win_tiles( x, y, 1, 1, buffer );
}

void setTiles( INT8 x, INT8 y, INT8 startTile, INT8 w ) {
    for( INT8 i = 0; i < w; i++ )
        setTile( x+i, y, startTile+i );
}

void setAllTiles( INT8 tile ) {
    for( INT8 y = 0; y < TH; y++ )
        for( INT8 x = 0; x < TW; x++ )
            setTile( x, y, tile );
}

void movePlayer( INT8 direction ) {
    player += direction;
    if( player < 0 )
        player = 0;
    else if( player >= ENEMIES )
        player = ENEMIES -1;

    SPRITE(SPRITE_PLAYER).y = player*8 + 16;
}

void updateHighscore( INT8 x, INT8 y ) {
    if( highscore == 0 ) {
        setTile( x, y, TILE_ZERO );
        return;
    }

    INT16 score = highscore;
    while( score > 0 ) {
        INT8 remainder = score % 10;
        score-=remainder;
        score/=10;
        setTile( x--, y, TILE_ZERO+remainder );
    }
}

INT8 numberWidth( INT16 number ) {
    INT8 w = 0;
    while( number > 0 ) {
        number /= 10;
        w++;
    }
    if( w == 0 )
        return 1;
    return w;
}

UINT8 randomEnemyX() {
    return 255 - (rand() & 0x3F);
}

void updateExplosions() {
    for( UINT8 i = 0; i < ENEMIES; i++ )
        if( explosions_time[i] > 0 ) {
            explosions_time[i]--;
            if( explosions_time[i] == 0 ) {
                ENEMY_SPRITE(i).tile = TILE_ENEMY;
                ENEMY_SPRITE(i).x = randomEnemyX();
            }
        }
}

void updateRocket() {
     if( shadow_OAM[SPRITE_ROCKET].x > 0 ) {
        scroll_sprite( SPRITE_ROCKET, 1, 0 );
        if( shadow_OAM[SPRITE_ROCKET].x > PW+8 )
            move_sprite( SPRITE_ROCKET, 0, 0 );
     }
}

INT8 enemyRepeat = 0;
void updateEnemies() {
    if( enemyRepeat-- > 0 )
        return;
    enemyRepeat = enemySpeed;

    for( UINT8 i = 0; i < ENEMIES; i++ ) {
        if( ENEMY_SPRITE(i).tile == TILE_EXPLOSION )
            continue;

        UINT8 x = ENEMY_SPRITE(i).x-1;
        ENEMY_SPRITE(i).x = x;

        if( x < 7 )
            gameRunning = 0;
    }
}

void killEnemy( INT8 enemy ) {
    NR44_REG = 0xC0; // TL-- ---- Trigger, Length enable

    ENEMY_SPRITE( enemy ).tile = TILE_EXPLOSION;
    explosions_time[enemy] = EXPLOSION_FRAMES;
}

INT8 shootRepeat = 0;
INT8 rocketY = -1;
void shoot() {
    /* check for rocket collisions */
    if( rocketY >= 0 ) {
        if( ENEMY_SPRITE( rocketY ).x <= shadow_OAM[SPRITE_ROCKET].x ) {
            highscore += 10;
            killEnemy( rocketY );

            rocketY = -1;
            move_sprite( SPRITE_ROCKET, 0, 0 );
        }
    }

    /* check for enemy collisions */
    if( shootRepeat >= 2 ) {
        if( ENEMY_SPRITE(shot).tile == TILE_ENEMY && ENEMY_SPRITE(shot).x < (PW + 6) ) {
            highscore++;
            killEnemy( shot );
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
    /* background */
    set_bkg_data( 0, NUMBER_OF_TILES, Tiles );
    fill_bkg_rect( 0, 0, TW, TH, 0 );

    /* window */
    move_win( 7, PH-8 );
    fill_win_rect( 0, 0, TW, TH, TILE_GRADIENT );

    /* sprites */
    set_sprite_data( SPRITE_ROCKET, NUMBER_OF_TILES, Tiles );
    set_sprite_tile( SPRITE_ROCKET, TILE_ROCKET );
    set_sprite_prop( SPRITE_ROCKET, 0x00 );
    move_sprite(     SPRITE_ROCKET, 0, 0 );

    for( UINT8 i = 0; i < ENEMIES; i++ ) {
        set_sprite_tile( SPRITE_ENEMIES + i, TILE_ENEMY );
        set_sprite_prop( SPRITE_ENEMIES + i, 0x00 );
        move_sprite(     SPRITE_ENEMIES + i, randomEnemyX(), 16 + i*8 );

        explosions_time[i] = 0;
    }

    set_sprite_tile( SPRITE_PLAYER, TILE_PLAYER );
    set_sprite_prop( SPRITE_PLAYER, 0x00 );
    move_sprite(     SPRITE_PLAYER, 8, 16 );
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
    SHOW_WIN;
    SHOW_SPRITES;
    DISPLAY_ON;

    while(1) {
        init();

        highscore = 0;

        while(gameRunning) {
            updateEnemies();
            updateExplosions();
            shoot();
            updateRocket();
            tileTarget = TARGET_WIN;
            updateHighscore( HIGHSCORE_X, HIGHSCORE_Y );
            tileTarget = TARGET_BKG;

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
                case J_LEFT:
                    gameRunning = 0;
                    break;
                case J_A:
                    if( shootRepeat == 0 ) {
                        shot = player;
                        shootRepeat = RAY_DURATION;
                    }
                    break;
                case J_B:
                    move_sprite( SPRITE_ROCKET, 13, (player+2)*8+1 );
                    rocketY = player;
                    break;
                default:
                    repeat = 0;
            }
            wait_vbl_done();
        }

        //printf("\n:;\n\n\n\n\nScore: %d\nGame Over, sorry :(\n\nHave a nice day ;)\n\nPress B to restart\n\n\n\n\n\n", highscore);
        setAllTiles( TILE_EMPTY );
        for( INT8 i = 0; i < ENEMIES; i++ )
            ENEMY_SPRITE(i).x = 0;

        setTiles( 5, 5, TILE_GAME_OVER, TILE_GAME_OVER_W );
        setTiles( 5, 6, TILE_SCORE, TILE_SCORE_W );
        setTile( 5 + TILE_SCORE_W, 6, TILE_EMPTY );
        updateHighscore( 5 + TILE_SCORE_W + numberWidth( highscore ), 6 );
        setTiles( 5, 8, TILE_PRESS_B, TILE_PRESS_B_W );

        while( joypad() != J_B ) wait_vbl_done;
        while( joypad() != 0   ) wait_vbl_done; // do not shoot rocket
    }
}
