#include <gb/gb.h>
#include <stdio.h>
#include "media/tiles.c"

// 160x144 px = 20x18 tiles

void main() {
    UINT8 i, j;

    set_bkg_data( 0, 4, Tiles );
    fill_bkg_rect( 0, 0, 4, 4, 0 );
    fill_bkg_rect( 4, 4, 4, 4, 1 );
    fill_bkg_rect( 8, 8, 4, 4, 2 );
    fill_bkg_rect( 12, 12, 4, 4, 3 );

    SHOW_BKG;
    DISPLAY_ON;

    while(1) {
        switch( joypad() ) {
            case J_LEFT:
                fill_bkg_rect( 12, 12, 4, 4, 3 );
                break;
            case J_RIGHT:
                fill_bkg_rect( 12, 12, 4, 4, 0 );
                break;
        }
        wait_vbl_done();
    }
}
