#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <rand.h>
#include "media/tiles.h"
#include "media/tiles.c"

#define NUMBER_OF_TILES 47
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
#define TILE_A 20
#define TILE_SCORE 20
#define TILE_SCORE_W 3
#define TILE_GAME_OVER 23
#define TILE_GAME_OVER_W 6
#define TILE_PRESS_B 29
#define TILE_PRESS_B_W 4
#define TILE_X 46

#define SPRITE(I) (shadow_OAM[I])

// TileColor: https://stackoverflow.com/a/1489985
#define PASTER( X, Y ) X ## Y
#define EVALUATOR( X, Y ) PASTER( X, Y )
#define TCOL( X ) EVALUATOR( TilesCGB, X )

#define REPEAT_FRAMES 4
#define EXPLOSION_FRAMES 42
#define LIFELOST_FRAMES 21
#define RAY_TEMP_PER_SHOT 5
#define RAY_TEMP_FAIL 51
#define RAY_TEMP_WARN 40
#define RAY_TEMP_COOL_FRAMES 10

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
#define ROCKETS_MAX 3
#define ASTROID_SPEED 2
#define RAY_DURATION 8

#define SPRITE_PLAYER 0
#define SPRITE_ROCKET 1
#define SPRITE_ENEMIES SPRITE_ROCKET+ROCKETS_MAX
#define ENEMY_SPRITE(I) SPRITE(SPRITE_ENEMIES+I)
#define ROCKET_SPRITE(I) SPRITE(SPRITE_ROCKET+I)

#define HIGHSCORE_X TW-1
#define HIGHSCORE_Y 0

#define LIFE_X 3
#define LIFE_Y HIGHSCORE_Y

#define ROCKETS_X 7
#define ROCKETS_Y HIGHSCORE_Y

#define RAY_TEMP_X 12
#define RAY_TEMP_Y HIGHSCORE_Y

// sound
#define STEPS_PER_BEAT 8
#define BEATS_PER_SEQUENCE 8
#define NUMBER_OF_SEQUENCES 8

INT8 gameRunning = -1;
INT8 explosions_time[ENEMIES];
INT8 player;
INT8 shot;
INT8 lifes;
INT8 rayTemperature;
INT8 rayLocked;
INT8 rockets;
INT8 buttonLockB;
INT8 rocketsY[ROCKETS_MAX];
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
        for( INT8 x = 0; x < BW; x++ )
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

void drawText( INT8 x, INT8 y, char *text ) {
    for (; *text != '\0'; text++){
        char c = *text;
        UINT8 tile = TILE_X;
        if( c == ' ' )
            tile = TILE_EMPTY;
        else if( '0' <= c && c <= '9' )
            tile = TILE_ZERO + (c-'0');
        else if( 'A' <= c && c <= 'L')
            tile = TILE_A + (c-'A');

        setTile( x++, y, tile );
    }
}

void drawHexNibble( INT8 x, INT8 y, INT8 value ) {
    if( value < 0xA )
        setTile( x, y, value + TILE_ZERO );
    else
        setTile( x, y, value - 0xA + TILE_A );
}

void drawHexByte( INT8 x, INT8 y, INT8 value ) {
    drawHexNibble( x    , y, value >> 4  );
    drawHexNibble( x + 1, y, value & 0xF );
}

void drawNumber( INT8 x, INT8 y, INT16 value, INT8 minWidth ) {
    if( value == 0 ) {
        while( minWidth-- > 0 )
            setTile( x--, y, TILE_ZERO );
        return;
    }

    while( value > 0 ) {
        INT8 remainder = value % 10;
        value-=remainder;
        value/=10;
        setTile( x--, y, TILE_ZERO+remainder );
        minWidth--;
    }
    while( minWidth-- > 0 )
        setTile( x--, y, TILE_ZERO );
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

void resetRocket( INT8 rocket ) {
    rockets--;
    move_sprite( SPRITE_ROCKET+rocket, 0, 0 );
    rocketsY[rocket] = -1;
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

void updateRockets() {
    for( INT8 i = 0; i < ROCKETS_MAX; i++ )
        if( ROCKET_SPRITE(i).x > 0 ) {
            scroll_sprite( SPRITE_ROCKET+i, 1, 0 );
            if( ROCKET_SPRITE(i).x > PW+8 )
                resetRocket(i);
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
        switch( tile ) {
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
                if( lifes >= 100 )
                    lifes = 99;
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
INT8 rayTempRepeat = 0;
void updateShot() {
    /* cool down ray */
    if( rayTempRepeat > 0 )
        rayTempRepeat--;
    else if( rayTemperature > 0 ) {
        rayTemperature--;
        rayTempRepeat = RAY_TEMP_COOL_FRAMES;
        if( rayLocked && rayTemperature == 0 ) {
            rayLocked = 0;
        }
    }

    /* check for rocket collisions */
    for( INT8 i = 0; i < ROCKETS_MAX; i++ )
        if( rocketsY[i] >= 0 ) {
            UINT8 tile = ENEMY_SPRITE(shot).tile;
            if( ( tile == TILE_ENEMY || tile == TILE_LIFE )
                && ENEMY_SPRITE( rocketsY[i] ).x <= ROCKET_SPRITE(i).x ) {
                highscore += 10;
                killEnemy( rocketsY[i] );

                resetRocket(i);
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

unsigned int timer_counter;
UINT8 beats[] = {
  //   1     2     3     4     5     6     7     8
    0xA3, 0xA3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // beat 0
    0xA3, 0xA3, 0x00, 0xA3, 0x00, 0x00, 0x00, 0x00, // beat 1
    0x93, 0x93, 0x00, 0x93, 0x00, 0x00, 0x00, 0x00, // beat 2
    0x93, 0x93, 0x00, 0x00, 0xC2, 0xC2, 0xC2, 0xC2, // beat 3
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // beat 4
    0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, // beat 5
    0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93  // beat 6
};

typedef struct beatSequences {
    UINT8 beats[STEPS_PER_BEAT];
    UINT8 repetitions;
} beatSequences_t;

// beats [0..7], number of runs (do not use 0)
beatSequences_t sequences[] = {
    { { 0, 0, 0, 0, 0, 0, 0, 0 }, 8 },
    { { 1, 1, 1, 1, 1, 1, 2, 3 }, 4 },
    { { 1, 1, 1, 1, 1, 1, 4, 4 }, 4 },
    { { 2, 2, 2, 2, 2, 2, 4, 4 }, 4 },
    { { 1, 1, 1, 1, 1, 1, 5, 5 }, 4 },
    { { 2, 2, 2, 2, 2, 2, 6, 6 }, 4 },
    { { 1, 1, 1, 1, 1, 1, 5, 5 }, 4 },
    { { 2, 2, 2, 2, 2, 2, 6, 6 }, 4 },
};

UINT8 beatStep = 0;
UINT8 beat = 0;
UINT8 sequence = 0;
UINT8 repetitions = 0;
void tim() {
    UINT8 index = sequences[sequence].beats[beat] * STEPS_PER_BEAT + beatStep;
    UINT8 note = beats[index];

    switch( note ) {
        case 0xA3:
            NR23_REG = 233; // A#3
            NR24_REG = 0x80;
            break;
        case 0x93:
            NR23_REG = 208; // G#3
            NR24_REG = 0x80;
            break;
        case 0xC4:
            NR23_REG = 15; // C#4
            NR24_REG = 0x81;
            break;
    }

    beatStep++;
    if( beatStep == STEPS_PER_BEAT ) {
        beat++;
        beatStep = 0;
    }
    if( beat == BEATS_PER_SEQUENCE ) {
        beat = 0;
        if( --repetitions == 0 ) {
            sequence++;
            repetitions = sequences[sequence].repetitions;
        }
    }
    if( sequence == NUMBER_OF_SEQUENCES )
        sequence = 0;
}

void init() {
    /* background */
    set_bkg_palette( 0, 8, spritePalette );
    set_bkg_data( 0, NUMBER_OF_TILES, Tiles );
    fill_bkg_rect( 0, 0, BW, BH, TILE_STARFIELD );


    /* window */
    move_win( 7, PH-8 );
    fill_win_rect( 0, 0, TW, TH, TILE_EMPTY );
    tileTarget = TARGET_WIN;
    setTile( LIFE_X-3, LIFE_Y, TILE_LIFE );
    setTile( LIFE_X-2, LIFE_Y, TILE_X );

    setTile( ROCKETS_X-2, ROCKETS_Y, TILE_ROCKET );
    setTile( ROCKETS_X-1, ROCKETS_Y, TILE_X );

    setTile( RAY_TEMP_X-3, RAY_TEMP_Y, TILE_RAY );
    setTile( RAY_TEMP_X-2, RAY_TEMP_Y, TILE_X );

    /* window-colors */
    VBK_REG=1;
    setAllTiles( 6 );
    VBK_REG=0;
    tileTarget=TARGET_BKG;

    /* sprites */
    set_sprite_palette( 0, 8, spritePalette );
    set_sprite_data( 0, NUMBER_OF_TILES, Tiles );
    for( INT8 i = 0; i < ROCKETS_MAX; i++ ) {
        set_sprite_tile( SPRITE_ROCKET+i, TILE_ROCKET );
        set_sprite_prop( SPRITE_ROCKET+i, TCOL( TILE_ROCKET ) );
        move_sprite(     SPRITE_ROCKET+i, 0, 0 );
        resetRocket(i);
    }
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
    rockets = 0;
    rayTemperature = 0;
    rayLocked = 0;
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

    INT8 wait = -1;
    // single note on square 2
    NR21_REG = 0x41; // DDLL LLLL Duty, Length load (64-L)
    NR22_REG = 0x81; // VVVV APPP Starting volume, Envelope add mode, period
    NR23_REG = 0x28; // FFFF FFFF Frequency LSB

    // Setup timer interrupts
    repetitions = sequences[0].repetitions;
    CRITICAL {
        add_TIM(tim);
    }
    TMA_REG=0xFF-102+1;
    TAC_REG=0x04U;
    //set_interrupts(TIM_IFLAG);

    while(wait) {
        switch( joypad() ) {
            case J_UP:
                //NR23_REG = 28;
                NR23_REG = 110; // A3
                //NR24_REG = 0x80;
                set_interrupts(TIM_IFLAG);
                break;
            case J_DOWN:
                //NR23_REG = 49;
                //NR23_REG = 196; // G3
                //NR24_REG = 0x80;
                set_interrupts(0);
                break;
            case J_RIGHT:
                NR23_REG = 62; // C4
                NR24_REG = 0x81;
                break;
            case J_A:
                wait = 0;
                break;
        }
    }
}

void updateWindow() {
    drawNumber( HIGHSCORE_X, HIGHSCORE_Y, highscore, 1 );
    drawNumber( LIFE_X, LIFE_Y, lifes, 2 );
    drawNumber( RAY_TEMP_X, RAY_TEMP_Y, rayTemperature, 2 );
    drawNumber( ROCKETS_X, ROCKETS_Y, ROCKETS_MAX-rockets, 1 );

    // change background of ray temperature
    VBK_REG=1;
    tileTarget = TARGET_WIN;
    INT8 palette = 6;
    if( rayLocked )
        palette = 3;
    else if( rayTemperature >= RAY_TEMP_WARN )
        palette = 1;
    for( INT8 i = 0; i < 4; i++ )
        setTile( RAY_TEMP_X-i, RAY_TEMP_Y, palette );
    tileTarget = TARGET_BKG;
    VBK_REG=0;

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
            updateShot();
            updateRockets();
            updateBackground();
            tileTarget = TARGET_WIN;
            updateWindow();
            tileTarget = TARGET_BKG;

            UINT8 buttons = joypad();
            if( (buttons & J_B) == 0 )
                buttonLockB = 0;
            switch( buttons ) {
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
                        if( rayLocked )
                            break;
                        rayTemperature += RAY_TEMP_PER_SHOT;
                        if( rayTemperature >= RAY_TEMP_FAIL ) {
                            rayLocked = -1;
                            break;
                        }

                        shot = player;
                        shootRepeat = RAY_DURATION;
                    }
                    break;
                case J_B:
                    if( buttonLockB || rockets >= ROCKETS_MAX )
                        break;
                    buttonLockB = -1;
                    rockets++;
                    for( INT8 i = 0; i < ROCKETS_MAX; i++ )
                        if( rocketsY[i] < 0 ) {
                            move_sprite( SPRITE_ROCKET+i, 13, (player+2)*8+1 );
                            rocketsY[i] = player;
                            break;
                        }
                    break;
                case J_START:
                    set_bkg_palette_entry( TCOL(TILE_STARFIELD), 0, RGB_PURPLE );

                    move_bkg( 0, 0 );
                    drawText( 0, 0, "HI1337 ABCDE" );

                    while( joypad() == J_START ) wait_vbl_done();
                    while( joypad() != J_START ) wait_vbl_done();

                    while( joypad() == J_START ) wait_vbl_done();
                    set_bkg_palette_entry( TCOL(TILE_STARFIELD), 0, RGB_BLACK );
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
        drawNumber( 5 + TILE_SCORE_W + numberWidth( highscore ), 6, highscore, 1 );
        setTiles( 5, 8, TILE_PRESS_B, TILE_PRESS_B_W );

        while( joypad() != J_START ) wait_vbl_done(); // wait for start
        while( joypad() == J_START ) wait_vbl_done(); // wait for release
    }
}
