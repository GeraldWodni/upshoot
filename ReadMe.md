# UpShoot

A simple space shooting game for the Gameboy

## Live on twitch
If you'd like to ask questions, or watch me code: all is done live on https://www.twitch.tv/4ther .

## Screenshots
![Rocket with star-background](/media/screenshots/upshoot-0.png?raw=true "Rocket fired with moving background, window highscore and sprites")

## Makefile / Build
Requires [GBDK](https://github.com/Zal0/gbdk-2020) to be present in `../gbdk` (can be configured in `Makefile`, `GBDK=../gbdk`).
Type `make` to build or `make run` to build and run with `mgba-qt` (can also be changed in `Makefile`).

## Tools used
- Tile-designer: [GBTD](http://www.devrs.com/gb/hmgd/gbtd.html)
- Emulator: `mgba-qt`, just because it worked out of the box on my rig, feel free to open an issue should you not be able to run it on your system or Gameboy
- Sound: [GBSoundDemo](https://github.com/Zal0/GBSoundDemo) very useful to quickly try out register settings and compare against your own code
