# Paddle Royale

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![SFML](https://img.shields.io/badge/SFML-3.0-green)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![License](https://img.shields.io/badge/License-MIT-green)

## Single-Player and Two-Player Brick Breaker with Competitive Faceoff Mode

## Table of Contents

* [Academic Information](#academic-information)
* [Project Overview](#project-overview)
* [Key Features](#key-features)
* [Technical Highlights](#technical-highlights)
* [Physics & Game Mechanics](#physics--game-mechanics)
* [Screenshots](#screenshots)
* [Project Structure](#project-structure)
* [Installation](#installation)
* [Documentation](#documentation)
* [Version History](#version-history)
* [Future Improvements](#future-improvements)
* [Contributors](#contributors)


Paddle Royale is a modern competitive reinterpretation of the classic Brick Breaker arcade game, developed in C++17 using SFML 3.0.0.

Unlike traditional single-player brick-breaking games, Paddle Royale introduces a local multiplayer experience where two players compete simultaneously, clear their respective brick fields, and ultimately face each other in a direct PvP showdown.

This project was developed as part of the CS-107: Computer Programming course at NUST SEECS.

---

## Academic Information

**Institution:** National University of Sciences and Technology (NUST)

**School:** School of Electrical Engineering and Computer Science (SEECS)

**Course:** CS-107: Computer Programming (Spring 2026)

**Faculty Advisor:** Engr. Shah Muhammad

### Development Team

* Zaka Ullah Khan (CMS ID: 542210)
* Muhammad Safi Ullah (CMS ID: 548923)
* Aakash Shahid Butt (CMS ID: 542098)
* Muhammad Saim (CMS ID: 541404)

---

## Project Overview

Paddle Royale transforms the traditional brick-breaking formula into a competitive two-player experience.

The game is built around a custom state-machine architecture that transitions between multiple gameplay phases:

* TITLE
* BREAKER
* WAITING
* FACEOFF
* GAMEOVER

Players compete to destroy their brick formations while strategically managing paddle size modifiers and life advantages that directly influence the outcome of the final faceoff.

---

## Key Features

### Breaker Mode

* Split-screen competitive gameplay
* Dual-player brick destruction system
* Dynamic brick grid generation
* Independent player progress tracking

### Power Brick System

#### Green Bricks

Increase the opponent's paddle size.

#### Red Bricks

Decrease the current player's paddle size.

This creates strategic risk-reward gameplay and constant player interaction.

### Waiting Phase

When one player clears their section first:

* A 60-second countdown begins
* The opposing player must clear remaining bricks
* Failure results in a life penalty before Faceoff Mode

### Faceoff Mode

After brick clearing:

* Central divider disappears
* Arena becomes fully shared
* Players battle directly
* Each player begins with six lives
* Last player standing wins

### Additional Features

- Local multiplayer support
- Single-player functionality
- Dynamic paddle scaling system
- Background music
- Audio controls and mute functionality
- Interactive menu interface
- Real-time keyboard input processing
- Multiple game states
- Full-screen gameplay experience

---

## Technical Highlights

### Language & Tools

* C++17
* SFML 3.0.0
* GCC 14.2.0 (MinGW UCRT 64-bit)

### Concepts Applied

* Object-Oriented Programming
* State Machines using Strongly-Typed Enums
* Functional Struct Design
* Lambda Expressions
* Collision Detection
* Real-Time Game Loop Architecture
* Event-Driven Input Handling
* Audio Management
* Dynamic Memory-Safe Design

---

## Physics & Game Mechanics

### Arena

* Resolution: 1920 × 1080
* Active Play Area: 1440 × 1080

### Paddle System

* Seven paddle size tiers
* Dynamic scaling mechanics
* Symmetric center-origin resizing

### Ball Physics

* Controlled velocity normalization
* Anti-stall bounce correction
* Stable angular collision response

### Brick Grid

* 12 × 12 dynamically generated matrix
* Individual brick size: 120 × 35 pixels

---

## Screenshots

### Main Menu

![Main Menu](screenshots/main-menu.jpg)

### Single Player Mode

![Single Player Mode](screenshots/single-player-mode.jpg)

### Two Player Mode

![Two Player Mode](screenshots/two-player-mode.jpg)

### Gameplay Mode

![Gameplay Mode](screenshots/gameplay-mode.jpg)

### Waiting Phase

![Waiting Phase](screenshots/waiting-phase.jpg)

### Faceoff Mode

![Faceoff Mode](screenshots/faceoff-mode.jpg)

### Pause Menu

![Pause Menu](screenshots/Pause-menu.jpg)

### End Game Screen

![End Game Screen](screenshots/end-game.jpg)


## Project Structure

```text
Brick-Breaker-Game/
├── Assets/
│   ├── ARIAL.TTF
│   └── Snowy.mp3
│
├── doc/
│   └── Project Report.pdf
│
├── screenshots/
│   ├── main-menu.jpg
│   ├── single-player-mode.jpg
│   ├── two-player-mode.jpg
│   ├── gameplay-mode.jpg
│   ├── waiting-phase.jpg
│   ├── faceoff-mode.jpg
│   ├── Pause-menu.jpg
│   └── end-game.jpg
│
├── src/
│   └── brickBreaker.cpp
│
├── .gitignore
|
├──README.md
└──LICENSE

```


## Installation

### Requirements

* C++17 Compatible Compiler
* SFML 3.0.0

### Build Command

```bash
g++ -std=c++17 src/brickBreaker.cpp -o PaddleRoyale ^
-I"path/to/SFML/include" ^
-L"path/to/SFML/lib" ^
-lsfml-graphics ^
-lsfml-window ^
-lsfml-audio ^
-lsfml-system
```

---

## Documentation

The complete project report and technical documentation are available in:

```text
doc/Project Report.pdf
```

The report contains:

* Project objectives
* Design methodology
* System architecture
* Gameplay mechanics
* Implementation details
* Testing and evaluation
* Conclusions and future enhancements

```
```

## Version History

| Version | Description                      |
| ------- | -------------------------------- |
| v5.0    | Final Release                    |
| v4.1    | Presentation Optimization        |
| v4.0    | Source Commenting & Bug Fixes    |
| v3.1    | Audio Controls & UI Improvements |
| v3.0    | Graphics & Audio Integration     |
| v2.1    | Competitive Faceoff Completion   |
| v2.0    | Gameplay Interaction Completion  |
| v1.5    | Color Logic System               |
| v1.4    | Grid Partition Mechanics         |
| v1.2    | Multiplayer Structure            |
| v1.1    | Structural Grid Tracking         |
| v1.0    | Base Game Framework              |

---

## Future Improvements

Potential future enhancements include:

* Online multiplayer support
* AI-controlled opponent
* Difficulty levels
* Power-up system
* Particle effects
* Achievement tracking
* Save and load functionality
* Enhanced UI animations

---

## Contributors

Developed as part of **CS-107: Computer Programming (Spring 2026)** at **NUST SEECS**.

### Team Members

* Muhammad Safi Ullah
* Zaka Ullah Khan
* Aakash Shahid Butt
* Muhammad Saim

## Documentation

The complete project report is available in:

```text
docs/Project_Report.pdf

```

## License

This project is licensed under the MIT License.

See the [LICENSE](LICENSE) file for details.

