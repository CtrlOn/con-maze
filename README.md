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
- [x] Multiple rooms
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
```ansi
Moves made:    0          Position:  8,  7,  0

########################################
##  G o o d   L u c k   ##            ##
##  ################  ######  ######  ##
##    ##              ##  ##      ##  ##
##  ######    ##  ##      ##  ##      ##
##          ####  ######  ##  ##########
##  ######  ##      ##    ##          ##
##  ##  ##  ##  <>  ##  ############  ##
##  ##[]##  ##      ##            ##  ##
##  ######  ##########  ########  ##  ##
##                  ##  ##[]o+##  ##  ##
##  ##########  ##  ##  ########  ##  ##
##          ##  ##  ##            ##  ##
########  ####  ##  ################  ##
##        ##            ##            ##
##  ########  ########  ########  ##  ##
##  ##[]##    ##    ##  ##      o+##  ##
##  ########  ########  ############  ##
##                                    ##
########################################


Use WASD to move, Q to quit.
```
  
### Indicators
```ANSI
Player:       <>
Goal:         []
Passage:      [] (Different colors)
Wall:         ##
Door:         ## (Different colors)
Key:          o+
```
  
### Instructions
- Player can be moved using keys `W` `A` `S` `D`
- Victory is achieved by stepping on goal
- Walls and doors prevent movement
- Goal is usually blocked by doors, which can be removed with key
- Collect the key by stepping on it, and doors will unlock
- Passages can bring player to other room
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

## Level building

File `level.dat` contains information about level.  
It can be edited with a text editor.  
The way it works is explained inside.

### level.dat
```ansi
; Welcome to level builder
; use unicode symbols to build levels
; ------------------------------------------------------------------------------------------------
; SYMBOL       METADATA    TILE        INFO
; ------------------------------------------------------------------------------------------------
; '#'          No          Wall        Impassable
; '&'          Yes         Door        Impassable until unlock
; '!'          Yes         Key         Collectible, unlocks doors of same ID
; ' '          No          Void        Passable
; '^'          Yes         Passage     Sends player to passage with same ID (must be 2 per ID)
; '@'          No          Start       Player start, passable
; '$'          No          Goal        Victory upon reaching
; a-zA-Z1-9    No          Text        Display only, passable
; ------------------------------------------------------------------------------------------------
; ID's are placed in metadata field, beyond the right side of room, ORDER MATTERS!
; Doors, keys, start has residue details (they act like Text)
; When player enters passage, it appears on top of next passage, to go back it will need to step off and enter back, make sure to leave room
; Do not let player escape map, case unhandled
; TIP: use insert mode to build rooms

; Level settings:
WIDTH 20

; Rooms:
; -------------------------------------
;         <- Tiles | Metadata ->
; -------------------------------------
BEGIN
####################
# Good Luck #      #
# ######## ### ### #
#  #       # #   # #
# ###  # #   # #   #
#     ## ### # #####
# ### #   #  #     #
# # & # @ # ###### # 2
# #$# #   #      # #
# ### ##&## #### # # 0
#         # #^!& # # 0 5 1
# ##&## # # #### # # 0
#     # # #      # #
#### ## # ###&#### # 0
#    #      #      #
# #### #### #### # #
# &^#  #  # &   !# # 0 1 0 0
# #### #### ###### #
#                  #
####################
####################
#    !    # #      # 6
# ##### #   # # #  #
# # ^ # # # # # #### 2
# & # & # #   #    # 6 6
# ####### # ###### #
#         #  #     #
#&############# ## # 2
#         #      # #
# ####### # #&# ## # 6
# #     #   #^#    # 0
# # ### #####&###### 5
# # # &!&         !# 4 3 4 1
#&# #&# #####&###### 5 4 5
# & &   #   # ##   # 3 3
# # #&### #      # # 4
# #^# #   ###### # # 1
# ###!# # #      # # 4
#     &     ####   # 5
####################
####################
#                  #
# ######### Nearly #
# # ^ #!!!# There  # 2 9 10 11
# #   #!!!#        # 12 13 14
# #   #&&&#####    # 15 15 15
# #       &   #    # 9
# #       &   #    # 9
# #       &   #    # 9
# #   #   #&&&#    # 10 10 10
# #   #   #&&&#    # 11 11 11
# # ! # ! #&&&#    # 7 8 12 12 12
# #   #   #&&&#    # 13 13 13
# #&&&#&&&#&&&#    # 8 8 8 7 7 7 14 14 14
# #   #   #   #    #
# #   # ! # ! #    # 15 2
# #   #   #   #    #
# #############    #
#                  #
####################
END
```

### Error handling
- There must exist 1 `Start`
- Keys that didn't open any doors log warning
- Passages that have no destination will cause non-fatal runtime error
- If line count is not multiple of `WIDTH` last unfinished room will be truncated
- Zero rooms levels will not load
- Unfinished row will autofill with `Wall`s and log warning
- Missing metadata will cause fatal error
- Excess metadata will ignore last and log warning

### Minimal example
```ansi
WIDTH 5
BEGIN
#####
#@ !# 0
#&&&# 0 0 0
#^  # 0
#####
#####
#$  #
#&&&# 1 1 1
#^ !# 0 1
#####
END
```
