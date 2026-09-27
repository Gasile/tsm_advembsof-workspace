# Advanced Embedded Software — Workspace (Project Phase A)

This repository contains the Zephyr RTOS T3 forest workspace and applications (`blinky` and `sanitize`) for the Advanced Embedded Software course.

---

## Codelab 2 — Question 1: Lookup Table Implementations & Sanitizer Analysis

The `sanitize` application implements a lookup table of size `kLutSize = 8` (`{10, 20, 30, 40, 50, 60, 70, 80}`) with `kOffset = 1` (`idx = sensor_value - 1`). Valid sensor values are `1` to `8` (indices `0` to `7`). Any `sensor_value <= 0` or `>= 9` produces an out-of-bounds index.

### 1. Observed Behavior Across Build Configurations (Out-of-Bounds Index)

| Implementation | Production Build (`just build sanitize ""`) | Debug Build (`just build sanitize debug`) | Sanitized Build (`just build sanitize san`) |
| :--- | :--- | :--- | :--- |
| **0: `lookup_c_assert`** | **Undefined Behavior (UB):** `ZPP_ASSERT` is compiled out (`CONFIG_ASSERT=n`). Reads arbitrary out-of-bounds memory (`kLut[idx]`) and prints garbage value without crashing. | **Controlled Halt:** `ZPP_ASSERT` triggers (`ASSERTION FAIL [idx >= 0 && idx < kLutSize]`), prints the diagnostic message with file/line and offending index, and halts the kernel. | **Runtime Trap:** Assertions are disabled, but UBSan (`-fsanitize=bounds -fsanitize=bounds-strict` + `CONFIG_UBSAN_TRAP=y`) catches the out-of-bounds array access at runtime and triggers a CPU trap / fatal fault. |
| **1: `lookup_c_safe`** | **Safe Clamping:** Index is clamped to `[0, kLutSize - 1]`. Returns `10` for `idx < 0` and `80` for `idx >= 8`. | **Safe Clamping:** Same as production build. Returns boundary value (`10` or `80`) without fault. | **Safe Clamping:** Same as production build. Since index is clamped before array access, UBSan is never triggered. |
| **2: `lookup_cpp`** | **Fatal Termination:** Calls `std::array::at(idx)`. Because exceptions are either disabled or uncaught, an out-of-bounds index triggers `std::out_of_range` / `std::terminate()`, halting the system. | **Fatal Termination:** Same as production build; halts immediately via `std::array::at()` bounds check. | **Fatal Termination:** Same as production build; `std::array::at()` checks bounds before raw array indexing occurs. |
| **3: `LookUpTable::lookup`** | **Graceful Error Handling:** Detects out-of-bounds `idx`, increments internal fault counter `s_count` (`record_fault()`), and returns `kFailSafeValue` (`-1`). | **Graceful Error Handling:** Same as production build. Returns `-1` and increments `s_count`. | **Graceful Error Handling:** Same as production build. Returns `-1` without triggering UBSan. |

---

### 2. Strengths and Weaknesses of Each Lookup Implementation

* **`lookup_c_assert` (C array + `ZPP_ASSERT`)**
  * **Strengths:** Zero runtime overhead in production builds; immediately catches contract violations during development/debugging with clear diagnostic logs.
  * **Weaknesses:** Unsafe for untrusted runtime inputs (such as hardware sensor readings) in production builds, because assertions disappear when `CONFIG_ASSERT=n`, leaving raw out-of-bounds memory accesses exposed.
* **`lookup_c_safe` (C array + index clamping)**
  * **Strengths:** Never crashes or accesses out-of-bounds memory in any build configuration; deterministic and fast; reasonable when saturating at minimum/maximum physical sensor limits makes domain sense.
  * **Weaknesses:** Silently masks sensor faults or wiring failures by returning valid-looking boundary data (`10` or `80`), making it impossible for the caller to distinguish a saturated error from a genuine boundary reading.
* **`lookup_cpp` (`std::array::at()`)**
  * **Strengths:** Enforces bounds checking in all build profiles (including production), preventing silent memory corruption or Undefined Behavior.
  * **Weaknesses:** Relies on C++ exceptions (`std::out_of_range`). In embedded real-time systems where exceptions are often disabled or uncaught, an out-of-bounds sensor spike causes an immediate system abort (`std::terminate`) rather than recoverable error handling.
* **`LookUpTable` (C++ class with fault counter and fail-safe return value)**
  * **Strengths:** Best suited for safety-critical embedded systems. Prevents out-of-bounds access in all builds, avoids exceptions, tracks diagnostic telemetry (`fault_count()`), and returns an explicit sentinel (`kFailSafeValue = -1`) so the caller can react safely.
  * **Weaknesses:** Slight runtime branching and storage overhead for fault counting; requires `-1` (`kFailSafeValue`) to be outside the range of valid lookup values (or should ideally return a `std::expected` / `std::optional`).

---

### 3. Additional Runtime Bug Detected by Sanitizer (Not Detected by `clang-tidy`)

In `sanitize/src/main.cpp`, option **`4` (`lookup_ubsan_bug`)** implements two classic Undefined Behaviors driven by the runtime `sensor_value`:
1. **Invalid Bit Shift (`-fsanitize=shift`):** Computes `1 << idx`. When `sensor_value` is `0` or negative (e.g., running `dec 4` from initial state), `idx` is negative (`-1` or `-2`), shifting by a negative exponent.
2. **Signed Integer Overflow (`-fsanitize=signed-integer-overflow`):** Computes `sensor_value * 1000000000`. When `inc 4` is called 3 times (`sensor_value = 3`), the 32-bit signed multiplication overflows `INT32_MAX` (`2,147,483,647`).

* **Why `clang-tidy` does not detect it:** `sensor_value` is a global `std::atomic<int32_t>` modified dynamically via interactive UART shell commands (`inc` / `dec`). Static analysis cannot know the runtime values passed to `lookup_ubsan_bug()`.
* **How the sanitizer detects it:** When compiled with `just build sanitize san` (`CONFIG_UBSAN=y`, `-fsanitize=shift`, `-fsanitize=signed-integer-overflow`), GCC instruments the arithmetic and shift instructions and immediately traps at runtime as soon as `dec 4` or 3x `inc 4` is entered in the shell.
