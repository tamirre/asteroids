# Asteroid Game

### [Try the web version here](https://gwarnak.itch.io/asteroids)
A simple Asteroid game written in C using the [raylib](https://www.raylib.com/) library. This is just a personal project to
learn the basics of game development with all its moving parts.
The player controls a spaceship and must dodge or destroy incoming asteroids to survive as long as possible.

## Gameplay

![](assets/scrn-rec-2026-04-08-13-15.gif)

## Getting Started

### Prerequisites

- A C compiler (e.g., msvc or gcc)
- [raylib](https://www.raylib.com/) library installed

### Installing raylib

Follow the [installation guide](https://github.com/raysan5/raylib#installation) for your operating system to set up raylib.

### Clone the Repository

```bash
git clone https://github.com/tamirre/asteroids.git
cd asteroids
```
### Building the Game (Linux)
```bash
./src/tools/build.sh
```
### Running the Game (Windows)
```bash
./src/tools/run.sh
```

## How to Play

- Use the **WASD** to move the rocket.
- Press the **spacebar** to shoot bullets.
- Press **P** to pause the game.
- Avoid or destroy asteroids to keep playing.
- The game ends if the player loses all hearts.

All assets are created using [Aseprite](https://www.aseprite.org/).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE.md) file for details.

## Acknowledgements

- [raylib](https://www.raylib.com/) for making game development in C simple and fun.
- Inspiration from classic arcade asteroid games.
