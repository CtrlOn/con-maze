# Console maze game

This is a game rendered with console monospaced characters
The goal is to escape by moving around with W/A/S/D
There are keys, doors and more

## Features

- [x] Render the maze and player
- [x] Character movement with W/A/S/D
- [x] Victory by reaching the goal
- [x] Keys and doors
- [x] Save file system
- [x] ANSI Colors for extra accesibilty
- [ ] Multiple rooms

## Gameplay

There's a game screen
```ANSI
Moves made: 0   Key: __

################################
##                ##  ##      ##
##  ()  ########  ##  ##  []  ##
##      ##  ##        @@      ##
##########  ########  ##########
##            ##      ##  ##  ##
##  ##    ##  ##  ##          ##
##  ####  ##      ##  ######  ##
##    ##@@####    ##      ######
##  ####    ############  ##  ##
##  ##      ##        ##      ##
##  ##  ##  @@    ##  ######  ##
##      ######    ##      ##  ##
##  ##  ##        ##  o+  ##  ##
##  ##        ##  ##      ##  ##
################################


Use WASD to move, Q to quit.
```

And these are indicators:
```
Player:       ()
Goal:         []
Wall:         ##
Door:         @@
Key:          o+
```

Player can be moved using keys `W` `A` `S` `D`
Victory is achieved by stepping on goal
Walls and doors prevent movement
Goal is usually blocked by doors, which can be removed with key
Collect the key by stepping on it, and doors will dissapear
Quit using key `Q`

## Save system

*Done playing before finishing?*
*Not a problem*

Upon quitting you will be prompted
`Would you like to save your progress? Y/N`
If `Y` is pressed, game will be stored in a `save.bin` file.

Upon launching the game again, you will be prompted
`Save file found! Continue? Y/N`
If `Y` is presseg, game will be restored.

Note:
Save is stored in moves, and rebuild deterministically

