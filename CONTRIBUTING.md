# Contributing to img2sample

## Getting Started

1. Fork the repository
2. Clone your fork
3. Build the plugin (see README.md for instructions)
4. Make your changes on a feature branch

## Development Setup

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

For UI work, the web app lives in `plugin/Resources/index.html` (plugin build) and
`web/index.html` (standalone browser build). Rebuild to re-bundle web resources
into the plugin artefacts.

## Code Style

- **C++**: Follow existing JUCE conventions (camelCase, JUCE types). Plugin code
  lives in `namespace img2sample`.
- **JS/HTML**: Match the existing inline style in `index.html`.
- **Shell**: Use `set -euo pipefail`, quote variables.

## Submitting Changes

1. Create a branch from `main`
2. Make focused, minimal commits
3. Test that the plugin builds and loads in a DAW
4. Open a pull request with a clear description of the change

## Reporting Issues

Open an issue with:
- What you expected to happen
- What actually happened
- Your OS and DAW (if applicable)
- Any error output

## License

By contributing, you agree that your contributions will be licensed under the GPLv3.
