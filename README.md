# Console maze game
  
- This is a game rendered with console monospaced characters  
- The goal is to escape by moving around with W/A/S/D  
- There are keys, doors and more  
  
## Features
  
- [x] Render the maze and player
- [x] Character movement with W/A/S/D
- [x] Victory by reaching the goal
- [x] Keys and doors
- [x] Save file system
- [x] ANSI Colors for extra accesibilty
- [ ] Multiple rooms
- [x] Error logging
- [ ] Leaderboard
  
## Usage
  
1. Download source code
2. Ensure there's `gcc` or `mingw` installed on your machine
3. Compile the code using `gcc main.c -o game.exe`
4. Launch `game.exe` either through explorer or terminal
5. ???
6. Profit

## Gameplay
  
### Game Screen
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
  
### Indicators
```ANSI
Player:       ()
Goal:         []
Wall:         ##
Door:         @@
Key:          o+
```
  
### Instructions
- Player can be moved using keys `W` `A` `S` `D`  
- Victory is achieved by stepping on goal  
- Walls and doors prevent movement  
- Goal is usually blocked by doors, which can be removed with key  
- Collect the key by stepping on it, and doors will dissapear  
- Quit using key `Q`
  
## Save system
  
> *Done playing before finishing?*  
> *Not a problem*  
  
Upon quitting you will be prompted  
`Would you like to save your progress? Y/N`  
If `Y` is pressed, game will be stored in a `save.bin` file.  
  
Upon launching the game again, you will be prompted  
`Save file found! Continue? Y/N`  
If `Y` is pressed, game will be restored.  
  
### Deterministic saving method
  
Game save is stored in moves made!  
This means that every input will be submitted one by one.  
Save file will grow with each move  
  
#### Why?
  
While technically it is possible to store players position, keys collected and etc. to save space, I preferred this method because:
- Instead of saving **Was the game won?** it will save **How was the game won?**
- It is possible to write a pathfinder algorithm that can beat the game without having to run it and fabricate the save file.
- Size per move is 1 `char`, which is negligible even for thousands of moves.
- No illegal placements if old file was used for a new map
- I get to add a cool loading screen
