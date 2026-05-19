# motor_suites

[![CMake on Linux with OpenGL](https://github.com/aconstlink/motor_suites/actions/workflows/cmake-lin-gcc-gl.yml/badge.svg)](https://github.com/aconstlink/motor_suites/actions/workflows/cmake-lin-gcc-gl.yml)
[![CMake on Win32 with DirectX 11](https://github.com/aconstlink/motor_suites/actions/workflows/cmake-win32-dx11.yml/badge.svg)](https://github.com/aconstlink/motor_suites/actions/workflows/cmake-win32-dx11.yml)

`motor_suites` contains sample applications and test scenarios for the [`motor`](https://github.com/aconstlink/motor) engine.

The repository is organized into suites that exercise individual engine layers such as application, audio, controls, format loading, geometry, graphics, math, memory, scene, tooling, and wire/data-flow systems.

Most suites are visual or manual integration tests rather than automated unit tests. They are used to validate engine behavior in realistic runtime scenarios and to provide compact examples for specific subsystems.

## Layout

- `motor/`  
  The engine repository, included as a submodule.

- `suite_*`  
  Focused sample and test applications for individual engine modules.

- `techniques/`  
  Experiments and technique-focused examples.

## Build

```bash
git clone --recursive https://github.com/aconstlink/motor_suites.git
cd motor_suites
cmake -S . -B build
cmake --build build
```

If the repository was cloned without submodules:

```bash
git submodule update --init --recursive
```

## Notes

The suites are intentionally separate from the engine repository. This keeps the engine focused while still providing a place for integration tests, visual checks, and examples that can evolve independently.
