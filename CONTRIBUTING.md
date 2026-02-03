# Contributing to NFS Share Manager

Thank you for your interest in contributing to NFS Share Manager! We welcome contributions from everyone.

## Code of Conduct

This project adheres to a code of conduct. By participating, you are expected to uphold this code:

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow
- Maintain a professional environment

## How to Contribute

### Reporting Bugs

1. **Check existing issues** first to avoid duplicates
2. **Use the bug report template** when creating new issues
3. **Provide detailed information**:
   - Operating system and version
   - Qt and KDE Frameworks versions
   - Steps to reproduce the issue
   - Expected vs actual behavior
   - Relevant log output

### Suggesting Features

1. **Check existing feature requests** to avoid duplicates
2. **Describe the use case** and why it would be valuable
3. **Provide mockups or examples** if applicable
4. **Consider implementation complexity** and maintenance burden

### Contributing Code

#### Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/yourusername/nfs-share-manager.git
   cd nfs-share-manager
   ```
3. **Set up the development environment**:
   ```bash
   # Install dependencies (see README.md)
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

#### Development Workflow

1. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**:
   - Follow the existing code style
   - Add tests for new functionality
   - Update documentation as needed

3. **Test your changes**:
   ```bash
   # Run all tests
   make test
   
   # Run specific tests
   ./tests/core/test_configurationmanager
   ./tests/business/test_sharemanager
   ```

4. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Add feature: brief description"
   ```

5. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create a Pull Request** on GitHub

#### Code Style Guidelines

- **C++ Standard**: Use C++17 features appropriately
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `ShareManager`)
  - Methods: `camelCase` (e.g., `createShare()`)
  - Variables: `camelCase` (e.g., `shareList`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `DEFAULT_TIMEOUT`)
  - Private members: `m_` prefix (e.g., `m_shareList`)

- **Code Organization**:
  - Header files: `.h` extension
  - Implementation files: `.cpp` extension
  - One class per file (generally)
  - Include guards or `#pragma once`

- **Documentation**:
  - Use Doxygen-style comments for public APIs
  - Include brief descriptions and parameter documentation
  - Document complex algorithms and business logic

- **Qt/KDE Guidelines**:
  - Follow Qt naming conventions
  - Use Qt containers and algorithms where appropriate
  - Prefer Qt signals/slots over callbacks
  - Use KDE Frameworks appropriately

#### Testing

- **Write tests** for new functionality
- **Update existing tests** when modifying behavior
- **Test categories**:
  - Unit tests: Test individual classes and methods
  - Integration tests: Test component interactions
  - End-to-end tests: Test complete workflows

- **Test structure**:
  ```cpp
  #include <QtTest/QtTest>
  
  class TestClassName : public QObject
  {
      Q_OBJECT
  
  private slots:
      void initTestCase();    // Run once before all tests
      void init();           // Run before each test
      void testMethodName(); // Individual test
      void cleanup();        // Run after each test
      void cleanupTestCase(); // Run once after all tests
  };
  ```

#### Documentation

- **Update README.md** for user-facing changes
- **Update inline documentation** for API changes
- **Add examples** for new features
- **Update build instructions** if dependencies change

### Pull Request Guidelines

#### Before Submitting

- [ ] Code follows the project style guidelines
- [ ] All tests pass locally
- [ ] New functionality includes tests
- [ ] Documentation is updated
- [ ] Commit messages are clear and descriptive

#### Pull Request Description

Include:
- **Summary** of changes made
- **Motivation** for the changes
- **Testing** performed
- **Screenshots** for UI changes
- **Breaking changes** if any

#### Review Process

1. **Automated checks** will run (build, tests, style)
2. **Maintainer review** for code quality and design
3. **Community feedback** may be requested
4. **Revisions** may be requested before merging

## Development Environment

### Required Tools

- **C++ Compiler**: GCC 9+ or Clang 10+
- **CMake**: 3.16 or later
- **Qt6**: 6.2 or later
- **KDE Frameworks 6**: Latest stable
- **Git**: For version control

### Recommended Tools

- **Qt Creator**: IDE with excellent Qt/CMake support
- **CLion**: JetBrains IDE with good CMake integration
- **VS Code**: With C++ and CMake extensions
- **Valgrind**: For memory debugging
- **GDB**: For debugging

### Build Configurations

#### Debug Build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

#### Release Build
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### With Tests
```bash
cmake -DBUILD_TESTING=ON ..
make -j$(nproc)
make test
```

## Project Structure

```
nfs-share-manager/
├── src/                    # Source code
│   ├── core/              # Core data structures
│   ├── business/          # Business logic
│   ├── system/            # System integration
│   └── ui/                # User interface
├── tests/                 # Test suite
│   ├── core/             # Core tests
│   ├── business/         # Business logic tests
│   ├── system/           # System tests
│   ├── ui/               # UI tests
│   └── integration/      # Integration tests
├── docs/                  # Documentation
├── policy/               # PolicyKit policies
└── releases/             # Release packages
```

## Getting Help

- **GitHub Issues**: For bugs and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Code Review**: Learn from feedback on pull requests
- **Documentation**: Check existing docs and code comments

## Recognition

Contributors will be:
- Listed in the project's contributor list
- Mentioned in release notes for significant contributions
- Invited to participate in project decisions

Thank you for contributing to NFS Share Manager! 🚀