#include <gb/gb.h>
#include <stdio.h>
#include "media/tiles.c"

#define TILE_EMPTY 0
#define TILE_PLAYER 1

// 160x144 px = 20x18 tiles
#define TW 20
#define TH 18

INT8 player;

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

void main() {
    set_bkg_data( 0, 8, Tiles );
    fill_bkg_rect( 0, 0, TW, TH, 0 );

    player = TH/2;
    movePlayer( 0 );

    SHOW_BKG;
    DISPLAY_ON;

    while(1) {
        switch( joypad() ) {
            case J_UP:
                movePlayer(-1);
                break;
            case J_DOWN:
                movePlayer(1);
                break;
        }
        wait_vbl_done();
    }
}
