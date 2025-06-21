# DUAL - Embedded Systems Project

This project is a multiplayer game inspired by the mobile game DUAL using two CC3200 microcontrollers, each connected to an OLED display. The game allows two players to battle each other as ships across devices by positioning their screens adjacent to each other. The individual boards communicate via a UART connection to synchronize game states. An accelerometer connected via I2C allows motion-based control, and a connected IR circuit with multi-tap functionality allows username input. Game scores and usernames are transmitted to a terminal display via AWS.

## Documentation
Full download and setup instructions are available at: [DUAL Webpage](https://dihan922.github.io/dual-webpage/)