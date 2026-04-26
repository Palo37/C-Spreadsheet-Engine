# 📊 SheetC

SheetC is a terminal-based spreadsheet engine written in C. It supports cell assignments, arithmetic expressions, and range-based functions, along with automatic dependency tracking and recalculation.

The program is designed to efficiently handle large spreadsheets while maintaining correctness through robust error handling and cycle detection.

---

## 🚀 Features

- 📌 Assign values to cells (e.g., `A1=10`)
- ➕ Arithmetic operations (`+`, `-`, `*`, `/`)
- 📐 Range-based functions:
  - `SUM`, `MIN`, `MAX`, `AVG`, `STDEV`
- ⏱️ `SLEEP` function for testing execution timing
- 🔄 Automatic recalculation of dependent cells
- 🔗 Dependency tracking using graph-based approach
- 🚫 Cycle detection to prevent infinite loops
- 📉 Error propagation (e.g., division by zero)
- 🧭 Scrollable interface for large sheets
- ⚡ Efficient updates (only affected cells recomputed)

---

## 🖥️ Usage

### Compile
```bash
make
