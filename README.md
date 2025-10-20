# Robotics 2nd homework
Number guessing arduino project
# Problem
For most of my life I've loved puzzle games, I really like playing card games like solitaire or FreeCell, I've also liked sharing that love with others, so when me and my girlfriend were bored we tried to make up numbers and guess them with small hints, I really liked this game so I wanted to be able to always play it.
# Design
The design of this game project is divided into four main parts, first we have the home screen from which you can either play or check the highscores, then we have the game screen where we play, from which if the game is won you can enter the name entry screen to add your score if it is good enough, finally the highscores show a list of the best player's names, how many tries it took for them to guess the number and the time it took to win.

# Parts List
| Component        | Quantity |
|------------------|----------|
| Arduino Uno      | 1        |
| Potenciometer    | 4        |
| Push button      | 2        |
| LCD Display      | 1        |
| 200 Ohm resistor | 3        |
| 330 Ohm resistor | 2        |

# Design and schematics

## Design
![Design](https://github.com/hyaqua/Robotika2/blob/main/Assets/design.jpg?raw=true)
## Schematics

# Demo video

# Encountered problems
- Keeping track of what should be on the screen, i created 2 char arrays to hold the current text which gets constantly redrawn to the screen, for easier tracking
- Using button interrupts, at first I put a lot of ifs inside my interrupts to handle the different cases for different menus and submenus, which was very time consuming for the interrupt and hard to manage, so I changed the code to handle everything inside the menus themselves
- LCD wasn't viewable, my screen contrast was very bad at first, but i managed to fix it by adding a second resistor in series
# Future improvements
- Add dificulty settings, like number length or maximum number of attempts
- Add sounds to the game, using a piezo
- Add multiplayer, where each person can come up with a number themselves and are able to play in turns
