# Contributing to mbpoll

First off, thank you for considering contributing to mbpoll! 🎉

## How to Contribute

### Reporting Bugs

- Check the [existing issues](https://github.com/epsilonrt/mbpoll/issues) to avoid duplicates.
- Use a clear and descriptive title.
- Describe the steps to reproduce the bug.
- Include your OS, mbpoll version (`mbpoll -V`), and any relevant logs.

### Suggesting Features

- Open an issue with the `enhancement` label.
- Explain the use case and why this feature would be useful.

### Pull Requests

1. **Fork** the repository.
2. **Create a branch** from `dev` (not `master`):
   ```bash
   git checkout dev
   git pull origin dev
   git checkout -b my-feature
   ```
3. **Make your changes** and commit with clear messages.
4. **Test your changes** — make sure the project builds on your platform.
5. **Push** your branch and open a PR **targeting `dev`**.

> ⚠️ **Important:** All pull requests must target the `dev` branch.  
> PRs against `master` will not be accepted, as `master` is reserved for stable releases only.

### Coding Guidelines

- Follow the existing code style (C99, 4-space indentation).
- Keep commits focused — one logical change per commit.
- Update documentation if your change affects usage or options.

### Build Instructions

See the [README](README.md#build-from-source) for detailed build instructions.

Quick summary:

```bash
mkdir build
cd build
cmake ..
make
```

### Running Tests

```bash
cd build
ctest
```

## Branch Strategy

| Branch    | Purpose                          |
|-----------|----------------------------------|
| `master`  | Stable releases only             |
| `dev`     | Active development, PR target    |

## License

By contributing, you agree that your contributions will be licensed under the [GPL-3.0 License](COPYING).
