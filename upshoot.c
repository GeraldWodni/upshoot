#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <rand.h>
#include "media/tiles.h"
#include "media/tiles.c"

#define NUMBER_OF_TILES 34
#define TILE_EMPTY 0
#define TILE_PLAYER 1
#define TILE_ROCKET 2
#define TILE_ENEMY 3
#define TILE_RAY 4
#define TILE_EXPLOSION 5
#define TILE_GRADIENT 6
#define TILE_STARFIELD 7
#define TILE_ASTROID 8
#define TILE_LIFE 9
#define TILE_ZERO 10
#define TILE_SCORE 20
#define TILE_SCORE_W 3
#define TILE_GAME_OVER 23
#define TILE_GAME_OVER_W 6
#define TILE_PRESS_B 29
#define TILE_PRESS_B_W 4
#define TILE_X 33

#define SPRITE_ROCKET 0
#define SPRITE_PLAYER 1
#define SPRITE_ENEMIES 2
#define SPRITE(I) shadow_OAM[I]
#define ENEMY_SPRITE(I) SPRITE(SPRITE_ENEMIES+I)

// TileColor: https://stackoverflow.com/a/1489985
#define PASTER( X, Y ) X ## Y
#define EVALUATOR( X, Y ) PASTER( X, Y )
#define TCOL( X ) EVALUATOR( TilesCGB, X )

#define REPEAT_FRAMES 4
#define EXPLOSION_FRAMES 42
#define LIFELOST_FRAMES 21

#define TARGET_BKG 0
#define TARGET_WIN 1

// 160x144 px = 20x18 tiles
#define PW 160
#define PH 144
#define TW 20
#define TH 18
#define BW 32
#define BH 32
#define ENEMIES TH-1
#define ENEMY_MIN_SPEED 8
#define ENEMY_MAX_SPEED 1
#define ASTROID_SPEED 2
#define RAY_DURATION 8

#define HIGHSCORE_X TW-1
#define HIGHSCORE_Y 0

#define LIFE_X 3
#define LIFE_Y HIGHSCORE_Y

INT8 gameRunning = -1;
INT8 explosions_time[ENEMIES];
INT8 player;
INT8 shot;
INT8 lifes;
UINT8 enemySpeed = ENEMY_MIN_SPEED;

INT16 highscore = 0;

INT8 tileTarget = TARGET_BKG;
void setTile( INT8 x, INT8 y, UINT8 tile ) {
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

void setAllTiles( UINT8 tile ) {
    for( INT8 y = 0; y < TH; y++ )
        for( INT8 x = 0; x < TW; x++ )
            setTile( x, y, tile );
}

UINT16 spritePalette[] = {
    /* 0 3 2 1 */
    TilesCGBPal0c0, TilesCGBPal0c1, TilesCGBPal0c2, TilesCGBPal0c3,
    TilesCGBPal1c0, TilesCGBPal1c1, TilesCGBPal1c2, TilesCGBPal1c3,
    TilesCGBPal2c0, TilesCGBPal2c1, TilesCGBPal2c2, TilesCGBPal2c3,
    TilesCGBPal3c0, TilesCGBPal3c1, TilesCGBPal3c2, TilesCGBPal3c3,
    TilesCGBPal4c0, TilesCGBPal4c1, TilesCGBPal4c2, TilesCGBPal4c3,
    TilesCGBPal5c0, TilesCGBPal5c1, TilesCGBPal5c2, TilesCGBPal5c3,
    TilesCGBPal6c0, TilesCGBPal6c1, TilesCGBPal6c2, TilesCGBPal6c3,
    TilesCGBPal7c0, TilesCGBPal7c1, TilesCGBPal7c2, TilesCGBPal7c3
};

void movePlayer( INT8 direction ) {
    player += direction;
    if( player < 0 )
        player = 0;
    else if( player >= ENEMIES )
        player = ENEMIES -1;

    SPRITE(SPRITE_PLAYER).y = player*8 + 16;
}

void drawNumber( INT8 x, INT8 y, INT16 value ) {
    if( value == 0 ) {
        setTile( x, y, TILE_ZERO );
        return;
    }

    while( value > 0 ) {
        INT8 remainder = value % 10;
        value-=remainder;
        value/=10;
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

void resetEnemy( INT8 enemy ) {
    INT8 r = rand();
    UINT8 tile;
    UINT8 prop;
    if( r > 0x70 ) {
        tile = TILE_LIFE;
        prop = TCOL( TILE_LIFE );
    }
    else if( r > 0x00 ) {
        tile = TILE_ASTROID;
        prop = TCOL( TILE_ASTROID );
    }
    else {
        tile = TILE_ENEMY;
        prop = TCOL( TILE_ENEMY );
    }

    ENEMY_SPRITE(enemy).tile = tile;
    ENEMY_SPRITE(enemy).x = randomEnemyX();
    ENEMY_SPRITE(enemy).prop = prop;
}

void updateExplosions() {
    for( UINT8 i = 0; i < ENEMIES; i++ )
        if( explosions_time[i] > 0 ) {
            explosions_time[i]--;
            if( explosions_time[i] == 0 ) {
                resetEnemy(i);
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

void updateBackground() {
    scroll_bkg( 1, 0 );
}

UINT8 lifeLostReset = 0;
INT8 lifeLost() {
    NR24_REG = 0xC1; // NR24 FF19 TL-- -FFF Trigger, Length enable, Frequency MSB

    // change background colors
    set_bkg_palette( 6, 1, &(spritePalette[7*4]) );
    lifeLostReset = LIFELOST_FRAMES;

    if( --lifes < 0 ) {
        gameRunning = 0;
        return 0;
    }
    return -1;
}

UINT8 frameCounter = 0;
void updateEnemies() {
    frameCounter++;
    for( INT8 i = 0; i < ENEMIES; i++ ) {
        UINT8 tile = ENEMY_SPRITE(i).tile;
        if( tile == TILE_EXPLOSION )
            continue;

        UINT8 x = ENEMY_SPRITE(i).x;
        switch( ENEMY_SPRITE(i).tile ) {
            case TILE_LIFE:
            case TILE_ASTROID:
                x+= -ASTROID_SPEED;
                break;
            case TILE_ENEMY:
                if( (frameCounter & 0x07) != 0 )
                    continue;
            default:
                x+= -1;
                break;
        }
        ENEMY_SPRITE(i).x = x;

        /* collision with astroid or enemy */
        if( x < 15 && player == i ) {
            if( tile == TILE_LIFE ) {
                lifes++;
                resetEnemy(i);
                continue;
            }
            if( lifeLost() ) {
                resetEnemy(i);
                continue;
            }
        }

        /* object past player */
        if( x < 7 ) {
            if( tile != TILE_ASTROID && tile != TILE_LIFE ) {
                if( lifeLost() ) {
                    resetEnemy(i);
                    continue;
                }
            }
            else if( x < ASTROID_SPEED )
                resetEnemy(i);
        }
    }
}

void killEnemy( INT8 enemy ) {
    NR44_REG = 0xC0; // TL-- ---- Trigger, Length enable

    ENEMY_SPRITE( enemy ).tile = TILE_EXPLOSION;
    ENEMY_SPRITE( enemy ).prop = TCOL(TILE_EXPLOSION);
    explosions_time[enemy] = EXPLOSION_FRAMES;
}

INT8 shootRepeat = 0;
INT8 rocketY = -1;
void shoot() {
    /* check for rocket collisions */
    if( rocketY >= 0 ) {
        UINT8 tile = ENEMY_SPRITE(shot).tile;
        if( ( tile == TILE_ENEMY || tile == TILE_LIFE ) && ENEMY_SPRITE( rocketY ).x <= shadow_OAM[SPRITE_ROCKET].x ) {
            highscore += 10;
            killEnemy( rocketY );

            rocketY = -1;
            move_sprite( SPRITE_ROCKET, 0, 0 );
        }
    }

    /* check for enemy collisions */
    if( shootRepeat >= 2 ) {
        UINT8 tile = ENEMY_SPRITE(shot).tile;
        if( (tile == TILE_ENEMY || tile == TILE_LIFE) && ENEMY_SPRITE(shot).x < (PW + 6) ) {
            highscore++;
            killEnemy( shot );
        }
    }

    /* draw ray & sound */
    if( shootRepeat == RAY_DURATION ) {
        NR14_REG = 0x87; // TL-- -FFF Trigger, Length enable, Frequency MSB
        for( UINT8 x = 0; x < BW; x++ )
            setTile( x, shot, TILE_RAY );
    }

    if( shootRepeat > 2 ) {
        shootRepeat--;
        return;
    }
    /* end ray */
    if( shootRepeat == 2 ) {
        shootRepeat--;
        for( UINT8 x = 0; x < BW; x++ )
            setTile( x, shot, TILE_STARFIELD );
        return;
    }
    /* wait for button release */
    if( shootRepeat == 1 && joypad() != J_A )
        shootRepeat = 0;
}

void init() {
    /* background */
    set_bkg_palette( 0, 8, spritePalette );
    set_bkg_data( 0, NUMBER_OF_TILES, Tiles );
    fill_bkg_rect( 0, 0, BW, BH, TILE_STARFIELD );


    /* window */
    move_win( 7, PH-8 );
    fill_win_rect( 0, 0, TW, TH, TILE_GRADIENT );
    tileTarget = TARGET_WIN;
    setTile( LIFE_X-3, LIFE_Y, TILE_LIFE );
    setTile( LIFE_X-2, LIFE_Y, TILE_X );
    setTile( LIFE_X-1, LIFE_Y, TILE_EMPTY );

    /* window-colors */
    VBK_REG=1;
    setAllTiles( 6 );
    VBK_REG=0;
    tileTarget=TARGET_BKG;


    /* sprites */
    set_sprite_palette( 0, 8, spritePalette );
    set_sprite_data( SPRITE_ROCKET, NUMBER_OF_TILES, Tiles );
    set_sprite_tile( SPRITE_ROCKET, TILE_ROCKET );
    set_sprite_prop( SPRITE_ROCKET, TCOL( TILE_ROCKET ) );
    move_sprite(     SPRITE_ROCKET, 0, 0 );

    for( UINT8 i = 0; i < ENEMIES; i++ ) {
        if( i==TH/2 ) {
            set_sprite_tile( SPRITE_ENEMIES + i, TILE_LIFE );
            set_sprite_prop( SPRITE_ENEMIES + i, TCOL( TILE_LIFE ) );
        }
        else {
            set_sprite_tile( SPRITE_ENEMIES + i, TILE_ENEMY );
            set_sprite_prop( SPRITE_ENEMIES + i, TCOL( TILE_ENEMY ) );
        }
        move_sprite(     SPRITE_ENEMIES + i, randomEnemyX(), 16 + i*8 );

        explosions_time[i] = 0;
    }

    set_sprite_tile( SPRITE_PLAYER, TILE_PLAYER );
    set_sprite_prop( SPRITE_PLAYER, TCOL( TILE_PLAYER ) );
    move_sprite(     SPRITE_PLAYER, 8, 16 );
    player = TH/2;
    movePlayer( 0 );

    lifes = 0;
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

void updateWindow() {
    drawNumber( HIGHSCORE_X, HIGHSCORE_Y, highscore );
    drawNumber( LIFE_X, LIFE_Y, lifes );

    // change background colors
    if( lifeLostReset > 0 && --lifeLostReset == 0 )
            set_bkg_palette( 6, 1, &(spritePalette[6*4]) );
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
            updateBackground();
            tileTarget = TARGET_WIN;
            updateWindow();
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

        move_bkg( 0, 0 );
        setTiles( 5, 5, TILE_GAME_OVER, TILE_GAME_OVER_W );
        setTiles( 5, 6, TILE_SCORE, TILE_SCORE_W );
        setTile( 5 + TILE_SCORE_W, 6, TILE_EMPTY );
        drawNumber( 5 + TILE_SCORE_W + numberWidth( highscore ), 6, highscore );
        setTiles( 5, 8, TILE_PRESS_B, TILE_PRESS_B_W );

        while( joypad() != J_B ) wait_vbl_done;
        while( joypad() != 0   ) wait_vbl_done; // do not shoot rocket
    }
}
