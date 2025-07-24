# DUAL! - Embedded Systems Multiplayer Game
A sophisticated multiplayer arcade game built on CC3200 microcontrollers, inspired by the mobile game DUAL. Players battle as ships across two separate devices with synchronized gameplay through UART communication.

## Features
- **Dual-Device Gameplay**: Two players battle simultaneously on separate CC3200 boards with OLED displays
- **Motion Control**: Accelerometer-based ship control via I2C interface for intuitive gameplay
- **Cross-Device Synchronization**: Real-time UART communication ensures seamless multiplayer experience
- **User Input System**: IR circuit with multi-tap functionality for username entry
- **Cloud Integration**: AWS connectivity for real-time score transmission and leaderboard display
- **Professional Graphics**: Custom Adafruit GFX implementation optimized for embedded displays
- **Responsive Controls**: Low-latency input processing for competitive gameplay

## Tech Stack
- **CC3200 Microcontroller** - Dual ARM Cortex-M4 with Wi-Fi connectivity
- **Adafruit SSD1351 OLED** - High-resolution color display with fast refresh rates
- **I2C Communication** - Accelerometer sensor integration for motion control
- **UART Protocol** - Inter-board communication for game state synchronization
- **IR Sensor Circuit** - Multi-tap input system for user interface
- **Flask Server** - Python backend for AWS integration and score management
- **Code Composer Studio** - Texas Instruments IDE for embedded development

## Getting Started

### Hardware Requirements
- 2x CC3200 LaunchPad development boards
- 2x Adafruit SSD1351 OLED displays
- 2x I2C accelerometer modules
- 2x IR sensor circuits
- UART connection cables
- Power supplies and breadboards

### Software Setup
1. **Install Code Composer Studio**: Download and install TI's CCS IDE
2. **Import Project**: Open the workspace in Code Composer Studio
3. **Configure Hardware**: Update pin configurations in `pin_mux_config.h` if needed
4. **Build Project**: Compile using the provided build configuration
5. **Flash Firmware**: Deploy to both CC3200 boards
6. **Setup Server**: Install Python dependencies and run the Flask server

### Quick Start
```bash
# Build the project (in Code Composer Studio)
Project → Build All

# Run the server component
pip install flask
python server.py

# Flash to CC3200 boards and connect displays
# Power on both devices and enjoy!
```

## Project Structure
```
dual-main/
├── main.c                  # Core game logic and main application loop
├── Adafruit_GFX.c         # Graphics library implementation
├── Adafruit_GFX.h         # Graphics library header and function declarations
├── Adafruit_OLED.c        # OLED display driver implementation
├── Adafruit_SSD1351.h     # SSD1351 OLED controller definitions
├── glcdfont.h             # Font definitions for text rendering
├── i2c_if.c               # I2C interface for accelerometer communication
├── pin_mux_config.c       # Hardware pin configuration and setup
├── pin_mux_config.h       # Pin mapping definitions and GPIO setup
├── cc3200v1p32.cmd        # Linker script for CC3200 memory mapping
├── server.py              # Flask server for AWS integration and scoring
├── README.md              # Project documentation
├── Debug/                 # Build artifacts and compiled binaries
│   ├── lab-final.bin      # Compiled firmware binary
│   ├── lab-final.map      # Memory map and symbol table
│   └── *.obj              # Object files and dependencies
└── targetConfigs/         # Code Composer Studio configuration
    ├── CC3200.ccxml       # Target configuration for debugging
    └── readme.txt         # Configuration documentation
```

## Hardware Setup
1. **OLED Display Connection**: Connect SSD1351 displays to SPI pins on each CC3200
2. **Accelerometer Wiring**: Connect I2C accelerometers to designated I2C pins
3. **UART Bridge**: Establish UART connection between the two CC3200 boards
4. **IR Sensor Setup**: Connect IR circuits for username input functionality
5. **Power Configuration**: Ensure stable power supply to all components

## Game Controls
- **Ship Movement**: Tilt accelerometer to control ship direction
- **Username Entry**: Use IR sensor multi-tap for character input
- **Game Start**: Both players must be connected and ready
- **Score Display**: Real-time scoring visible on both displays and server

## Server Integration
The Flask server (`server.py`) provides:
- Real-time score collection from both devices
- Player username management
- AWS cloud integration for data persistence
- Terminal display for spectators and scorekeeping

### Server Configuration
```bash
# Install dependencies
pip install flask

# Run development server
python server.py

# Server runs on localhost:5000 by default
# Configure CC3200 Wi-Fi to connect to your network
```

## Development
### Building from Source
1. Open project in Code Composer Studio
2. Configure build settings for CC3200 target
3. Compile all source files
4. Generate binary for flashing

### Debugging
- Use Code Composer Studio's integrated debugger
- UART output available for runtime debugging
- LED indicators for connection status

### Adding Features
- Modify `main.c` for core game logic changes
- Update graphics in `Adafruit_GFX.c` for visual enhancements
- Extend server functionality in `server.py` for new features

## Deployment
1. **Hardware Assembly**: Connect all components according to wiring diagrams
2. **Firmware Flash**: Deploy compiled binary to both CC3200 boards
3. **Network Setup**: Configure Wi-Fi credentials for AWS connectivity
4. **Server Launch**: Start Flask server on accessible network endpoint
5. **Game Ready**: Power on devices and begin multiplayer battles!

## Documentation
Full setup instructions, wiring diagrams, and gameplay tutorials available at: [DUAL Webpage](https://dihan922.github.io/dual-webpage/)

## Troubleshooting
- **Connection Issues**: Verify UART wiring between boards
- **Display Problems**: Check SPI connections and power supply
- **Sensor Calibration**: Ensure accelerometers are properly mounted
- **Network Connectivity**: Verify Wi-Fi credentials and server accessibility
