#include <gb/gb.h>
#include <stdio.h>
#include <rand.h>
#include "media/tiles.c"

#define TILE_EMPTY 0
#define TILE_PLAYER 1
#define TILE_ROCKET 2
#define TILE_ENEMY 3
#define TILE_RAY 4

#define REPEAT_FRAMES 4

// 160x144 px = 20x18 tiles
#define TW 20
#define TH 18
#define ENEMIES TH
#define ENEMY_MIN_SPEED 40
#define ENEMY_MAX_SPEED 1
#define RAY_DURATION 8

INT8 gameRunning = -1;
INT8 enemies[ENEMIES];
INT8 player;
INT8 shot;
INT8 enemySpeed = ENEMY_MIN_SPEED;

void setTile( INT8 x, INT8 y, INT8 tile ) {
    unsigned char buffer[1] = { tile };
    set_bkg_tiles( x, y, 1, 1, buffer );
}

void movePlayer( INT8 direction ) {
    INT8 lastPlayer = player;
    player += direction;
    if( player < 0 )
        player = 0;
    else if( player >= TH )
        player = TH -1;

    setTile( 0, lastPlayer, TILE_EMPTY );
    setTile( 0, player, TILE_PLAYER );
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
    return (rand() & 0x1F) + TW + RAY_DURATION*2;
}

INT8 shootRepeat = 0;
void shoot() {
    /* check for enemy collisions */
    if( shootRepeat >= 2 ) {
        if( enemies[shot] < TW ) {
            enemies[shot]=randomEnemyX();
        }
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
    set_bkg_data( 0, 8, Tiles );
    fill_bkg_rect( 0, 0, TW, TH, 0 );

    for( UINT8 i = 0; i < ENEMIES; i++ )
        enemies[i] = randomEnemyX();

    player = TH/2;
    movePlayer( 0 );

    gameRunning = -1;
}

void main() {
    UINT8 repeat = 0;

    SHOW_BKG;
    DISPLAY_ON;

    while(1) {
        init();

        while(gameRunning) {
            updateEnemies();
            shoot();

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
        printf(":;\n\n\n\n\n\n\nGame Over, sorry :(\n\nHave a nice day ;)\n\nPress B to restart");
        while( joypad() != J_B ) wait_vbl_done;
    }
}
