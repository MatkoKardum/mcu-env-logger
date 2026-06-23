# Contributing to MCU Environmental Data Logger

Thanks for your interest in contributing! This is a community-driven project and contributions are welcome.

## How to Contribute

### Report a Bug
- Check existing [Issues](../../issues) to avoid duplicates
- Create a new issue with:
  - Clear title describing the problem
  - Steps to reproduce
  - Expected vs actual behavior
  - ESP32 board model, Arduino IDE version

### Suggest an Enhancement
- Discuss your idea in [Issues](../../issues) first
- Provide use cases and benefits
- Examples of similar implementations are helpful

### Submit Code
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes
4. Test thoroughly on your ESP32 board
5. Commit with clear messages: `git commit -m "Add support for DHT11 sensor"`
6. Push to your fork and create a Pull Request

### Code Style
- Follow the existing code formatting
- Keep functions modular and well-commented
- Test code on actual ESP32 hardware before submitting
- Avoid breaking changes if possible

## Development Setup

1. Clone the repository
2. Install Arduino IDE or PlatformIO
3. Install dependencies listed in README.md
4. Copy `config.example.h` to `config.h`
5. Configure for your hardware
6. Test on your ESP32 board

## Questions?
- Open an issue with [question] tag
- Check existing documentation in README.md and COMPLETE_USER_GUIDE.md

Thank you for contributing! 🎉
