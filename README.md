# eli - shell

**eli** is a simple, lightweight shell (command-line interpreter) written in **C++**. This project is designed to help understand basic shell concepts, command processing, and process management on Unix-like operating systems.

## Features (Current / In Progress)

- Execute basic commands (e.g., `ls`, `pwd`, `cd`)
- Support for running external programs
- Simple command prompt display
- Basic error handling

> **Note:** This project is in its early stages of development and may not yet have all features of a full-featured shell.

## File Structure

- `eli.cpp` : Main file containing the shell loop and core logic
- `extra.hpp` : Helper functions (e.g., tokenizing input, string manipulation)
- `head.hpp` : Headers, structure definitions, and constants

## Build and Run

Follow these steps in your terminal:

```bash
# Clone the repository
git clone https://github.com/nimacpp/eli.git
cd eli

# Compile (using g++)
g++ -o eli eli.cpp

# Run
./eli
