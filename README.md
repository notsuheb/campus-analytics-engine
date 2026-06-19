# Campus Analytics Engine

A command-line, menu-driven analytics system for a fictional university campus,
written in **C++ using only fundamental programming constructs** — no STL
algorithms, no `map`/`set`, no `class`, and no `<ctime>`. All data is loaded
from and written back to plain-text (`.txt`) files.

This is the Programming Fundamentals project for BS Artificial Intelligence.

---

## Features

The program is organised as a **3-level nested menu** (main menu → module menu →
action). Each menu option calls a function in one of seven modules:

| Module | File | What it does |
| ------ | ---- | ------------ |
| M1 | `filehandler` | Generic TXT read/write: character-by-character CSV parser (handles quoted commas like `"Khan, Ahmed"`), overwrite, append, linear search. |
| M2 | `student_ops` | Add / search / update / soft-delete students; `BSAI-YY-XXX` roll validation; selection-sort listing; **live search-as-you-type** (bonus). |
| M3 | `course_ops` | Enroll / drop courses, seat & duplicate checks, 21-credit limit, prerequisite verification. |
| M4 | `attendance` | Mark P/A/L, compute `(present + 0.5*late)/total` %, flag <75% shortage, undo last session. |
| M5 | `grades` | Enter marks, best-three-of-five quizzes, weighted total, letter grade, credit-weighted GPA, class statistics, attendance penalty. |
| M6 | `fee_tracker` | Record payments, **manual date arithmetic** (no `<ctime>`), 2%-per-week late fines, formatted receipts, bubble-sorted defaulters. |
| M7 | `reports` | Merit list, attendance defaulters, fee defaulters, semester result sheet, department summary, export any report to a `.txt` file. |
| M0 | `main` | Entry point and the nested menu that ties everything together. |

---

## Project structure

```
campus-analytics-engine/
├── main.cpp              # M0 - menu driver
├── filehandler.h/.cpp    # M1
├── student_ops.h/.cpp    # M2
├── course_ops.h/.cpp     # M3
├── attendance.h/.cpp     # M4
├── grades.h/.cpp         # M5
├── fee_tracker.h/.cpp    # M6
├── reports.h/.cpp        # M7
├── data/
│   ├── students.txt
│   ├── courses.txt
│   ├── enrollments.txt
│   ├── attendance_log.txt
│   └── fees.txt
├── Makefile
└── README.md
```

> `data/grades.txt` is **created automatically** by the grades module the first
> time you enter marks, so it is not shipped with the project.

---

## How to compile

You need a C++17-capable compiler (g++ or clang++).

**Using the Makefile (recommended):**

```bash
make
```

**Or compile manually:**

```bash
g++ -std=c++17 main.cpp filehandler.cpp student_ops.cpp course_ops.cpp \
    attendance.cpp grades.cpp fee_tracker.cpp reports.cpp -o campus_engine
```

## How to run

Run the program **from the project root** so that the `data/` paths resolve
correctly:

```bash
./campus_engine
```

---

## Sample run

```
====================================================
        CAMPUS ANALYTICS ENGINE  (BS AI)
====================================================

  ===== MAIN MENU =====
   1. Student Management
   2. Course Management
   3. Attendance
   4. Grades
   5. Fees
   6. Reports
   0. Exit
  Choice: 6

  -- Reports --
   1. Merit list
   ...
  Choice: 1

  MERIT LIST (active students by CGPA)
  ================================================
  Rank  Roll          Name                  CGPA
  ------------------------------------------------
  1     BSAI-23-006   Sana Pervez           3.90
  2     BSAI-23-012   Maham Javed           3.80
  3     BSAI-23-001   Ahmed Raza            3.75
  ...
```

---

## Design notes 

* **No STL algorithms.** Sorting is hand-written: selection sort
  (`listActiveStudents`, median in `computeClassState`) and bubble sort
  (`printMeritList`, `getDefaulters`). Searching is a linear scan (`findRow`).
* **No `map`/`set`.** Grouping in `printDepartmentSummary` uses *parallel arrays*
  plus a linear lookup instead of an associative container.
* **No `<ctime>`.** Dates are `DD-MM-YYYY` strings parsed by hand; `daysBetween`
  converts each date to a running day count using a month-length array that is
  leap-year aware.
* **File I/O.** `readTXT` parses one character at a time (not getline-and-split)
  and tracks an `inQuotes` flag so commas inside quoted fields are preserved.
* **Column indices** are named constants (e.g. `STU_CGPA`, `FEE_TOTAL`) so there
  are no "magic numbers" in the logic.

## Marks weighting reference

* Quiz 10% + Assignment 10% + Mid 30% + Final 50% (all converted to a percentage
  before weighting).
* Grade scale: `>=85 A`, `>=80 B+`, `>=70 B`, `>=65 C+`, `>=60 C`, `>=50 D`,
  else `F`.
* A student below 75% attendance in a course is forced to an `F` for it.
