# Asteroid Game

A simple Asteroid game written in C using the [raylib](https://www.raylib.com/) library. The player controls a spaceship and must dodge or destroy incoming asteroids to survive as long as possible.

## Gameplay

![](https://github.com/tamirre/asteroids/assets/asteroids.gif)

## Getting Started

### Prerequisites

- A C compiler (e.g., GCC or Clang)
- [raylib](https://www.raylib.com/) library installed

### Installing raylib

Follow the [installation guide](https://github.com/raysan5/raylib#installation) for your operating system to set up raylib.

### Clone the Repository

```bash
git clone https://github.com/tamirre/asteroids.git
cd asteroids
```

<!-- ## Building the Game -->

<!-- ### On Linux/MacOS

1. Compile the source code:
   ```bash
   gcc -o asteroid_game asteroid_game.c -lraylib -lm -ldl -pthread
   ```
2. Run the game:
   ```bash
   ./asteroid_game
   ```

### On Windows

1. Open a terminal and navigate to the project directory.
2. Compile the source code:
   ```bash
   gcc -o asteroid_game asteroid_game.c -lraylib
   ```
3. Run the game:
   ```bash
   asteroid_game.exe
   ``` -->

## How to Play

- Use the **WASD** to move the rocket.
- Press the **spacebar** to shoot bullets.
- Avoid or destroy asteroids to keep playing.
- The game ends if the player loses all three hearts.

## Code Structure

- `asteroids.c`: Main game logic, including initialization, game loop, and rendering.
- `assets/`: Directory for optional assets like images or sounds.

All assets are created using [Aseprite](https://www.aseprite.org/).


## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [raylib](https://www.raylib.com/) for making game development in C simple and fun.
- Inspiration from classic arcade asteroid games.
