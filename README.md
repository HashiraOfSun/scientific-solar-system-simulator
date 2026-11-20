# Scientific Interactive 3D Solar System Simulator

An updated educational project built with C++17, GLFW, GLM, macOS OpenGL 4.1 Core, and GLSL 410.

**Getting started:** this is a complete project directory, not a code snippet to paste into `main.cpp`. Copy or extract the entire directory.

## 1. Building and Running on macOS

Install the required dependencies if they are not already available:

```sh
brew install cmake glfw glm
```

Open Terminal in the project directory and run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/SolarSystem
```

Alternatively, double-click `Launch.command`. If macOS prevents the downloaded file from opening, use the commands above.

Do not copy an existing `build` directory when moving the project: CMake stores absolute paths in its cache.

After building, application resources are placed alongside the executable, allowing it to run from another working directory. When transferring the source project, include all files, including `textures`, `shaders`, and `external`. The compiled executable requires its texture and shader directories and compatible runtime dependencies. It is not a standalone `.app` bundle.

The simulation starts **paused at 2000-01-01 12:00 TDB (J2000.0)**. Dates represent TDB, not local time or UTC.

## 2. First Steps

1. Press **Space** to start the simulation. The default speed is 10 simulated days per real second.
2. Press **1** to select Mercury and follow it with the camera. Press **R** to view the inner planetary orbits.
3. Select Earth with **3** and inspect its information panel. Physical values are independent of the display scale.
4. Press **T**, enter `2026-10-21`, and press **Enter**. Date entry pauses the simulation and sets the time to 00:00 TDB.
5. Press **V** to view the entire system. Outer planets move much more slowly than inner planets. Increase the simulation speed with **+**.
6. Press **M** to enable physical scale. Planets become very small in the overview; select a planet to inspect it. Colored markers and labels indicate positions, not physical sizes.
7. Select Uranus with **7** and enable axes with **X**. Its directed axial tilt exceeds 90°. The rotation period follows NASA’s negative-period convention; the code does not reverse the rotation a second time.

## 3. Controls

| Action | Keyboard / mouse |
|---|---|
| Pause / resume | Space or Play/Pause |
| Increase / decrease simulation speed | `+` / `-`; doubles or halves the speed, from 1 second/second to 4096 days/second |
| Reverse time | B |
| Step backward / forward by one day | `[` / `]` or corresponding buttons; pauses the simulation |
| Enter a date | T, YYYY-MM-DD, Enter; Esc cancels |
| Return to J2000 | J |
| Select and track a body | 0: Sun; 1–8: planets; body-list buttons or scene markers |
| Zoom in / out | Scroll wheel, scrolling gesture, W / S |
| Rotate the camera | Right mouse drag; A / D; up / down arrows |
| Refocus on the selected body | F |
| Inner system / entire system | R / V |
| Physical scale / exaggerated radii | M |
| Decrease / increase radii | `,` / `.`; demonstration mode only |
| Change scene units per AU | K / P; camera distance adjusts accordingly |
| Toggle orbits / axes / labels | O / X / L |
| Exit | Esc outside date entry |

The camera orbits a selected tracking target; free-flight WASD navigation is not implemented.

Simulation time stops accumulating while the window is unfocused. Frame delays longer than 0.5 seconds are ignored so that waking the computer does not advance the simulation by years.

## 4. Main Improvements

| Previous implementation | Updated implementation |
|---|---|
| Arbitrary distance and speed values | JPL coefficients, AU, days, and the J2000 epoch |
| Coplanar circular orbits | Elliptical orbits with eccentricity, inclination, ascending node, and perihelion |
| Angle calculated as speed × time | Mean anomaly → Kepler’s equation → position |
| Single floating-point timer | Double-precision JD TDB, pause, speed control, reverse time, and calendar dates |
| Arbitrary body sizes | NASA radii with separately defined display exaggeration |
| Shared rotation axis | Physical axial tilts and rotation periods with a simplified orientation model |
| Overview only | Body selection, tracking, information panels, current distance, and calculated speed |
| No scientific validation | 286 checks, CSV export, and a Horizons comparison tool |

By default, distances are linear: **12 scene units = 1 AU**. Planetary radii are exaggerated by a factor of 250 and the Sun’s radius by a factor of 5.

Physical-scale mode applies a consistent scale to all spheres. Changing scene units with K/P also adjusts the camera distance.

## 5. Code Structure

- `Planet.h/.cpp`: body catalog and sphere geometry. Entries contain physical parameters, units identified by field names, and orbital elements with their rates of change.
- `Animation.h/.cpp`: double-precision calculations, calendar handling, simulation clock, and physical-to-scene conversions. Independent of OpenGL.
- `main.cpp`: GLFW integration, camera, input, selection, rendering, export, and offscreen validation.
- `Hud.h/.cpp`: information panels and buttons rendered with OpenGL triangles, without an additional UI library.
- `Shader.*`, `Texture.*`, `shaders/`: shader and texture loading. The original Phong lighting model is retained; loading failures stop startup.
- `tests/science_tests.cpp`: scientific and calendar checks.
- `data/planetary_data.csv`: a readable copy of the parameters. The application uses the catalog in `Planet.cpp` rather than loading this CSV.

## 6. Export and Reproducibility

```sh
./build/SolarSystem --date 2026-10-21 --export state.csv
./build/SolarSystem --select 6
./build/SolarSystem --select 3 --true-scale
./build/SolarSystem --overview
./build/SolarSystem --offscreen --screenshot scene.ppm
```

CSV export does not create a window. Coordinates are heliocentric, expressed in the J2000 ecliptic frame, with distances in AU and time in JD TDB.

`data/example_2026-10-21.csv` was generated by this application. It is **not an independent NASA reference dataset**.

An independent comparison can be requested when network access is available:

```sh
python3 tools/compare_horizons.py state.csv --output horizons-check
```

The script queries the official JPL API and saves the original responses and an error table. Use `--cached` to repeat the comparison with saved responses.

A live API comparison was not completed during the original validation because network access was unavailable. The script had not been verified against a live response at that stage.

## 7. Validation and Limitations

The original macOS arm64 validation completed compilation and linking successfully. One CTest suite passed **286 checks**, and CSV export produced data for nine bodies. See `docs/VALIDATION.md`.

Window creation and GPU rendering were not confirmed during that original validation. The execution environment could not access the macOS window service, and the offscreen OpenGL attempt could not obtain a suitable pixel format. This was a validation limitation, not proof that GPU rendering worked correctly. The manual verification procedure is documented in `docs/VALIDATION.md`.

This is a scientifically informed educational approximation, not an N-body integrator or a replacement for JPL Horizons.

Axial tilts use physical values, but pole azimuths and initial texture phases are illustrative. The model should not be used to determine sunrise times, seasons, transits, or eclipses.

The Moon is not modeled separately. The Earth sphere is positioned at the Earth–Moon barycenter. Further details are available in `docs/SCIENCE.md`.

## 8. Development Plan and Course Materials

See `docs/PLAN_RU.md` for the source review, syllabus mapping, schedule, milestones, team roles, and presentation outline. The document distinguishes completed features from remaining work.

## 9. Data Sources and Credits

Planetary data comes from NASA/JPL. Direct references are listed in `docs/SCIENCE.md`.

Textures are inherited from the original project and provided by **Solar System Scope / INOVE** under **CC BY 4.0**. See `docs/CREDITS.md` for attribution and licensing details.
