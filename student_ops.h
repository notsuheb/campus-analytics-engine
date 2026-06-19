#ifndef STUDENT_OPS_H
#define STUDENT_OPS_H

// ============================================================================
// M2 - student_ops.h
// ----------------------------------------------------------------------------
// Everything about a STUDENT record: add, search, update, soft-delete and list.
// Depends only on M1 (filehandler) for the actual reading/writing.
// ============================================================================

#include <string>
#include <vector>
#include "filehandler.h"

// Column positions inside students.txt. Naming them avoids "magic numbers"
// like row[4] scattered through the code, and makes the viva explanation easy.
const int STU_ROLL   = 0;
const int STU_NAME   = 1;
const int STU_DEPT   = 2;
const int STU_SEM    = 3;
const int STU_CGPA   = 4;
const int STU_STATUS = 5;

// A typed view of one student row. Using a struct (NOT a class) is exactly what
// the PDF asks for.
struct Student {
    std::string roll;
    std::string name;
    std::string department;
    int         semester;
    double      cgpa;
    std::string status;   // "active" or "inactive"
};

// ---- Conversion helpers between a raw Row and a typed Student --------------
Student rowToStudent(const Row& r);
Row     studentToRow(const Student& s);

// ---- Validation (all done with plain string/char logic, no regex) ----------
bool isValidRollFormat(const std::string& roll);   // BSAI-YY-XXX
bool nameHasNoDigits(const std::string& name);
bool isValidCgpa(double cgpa);                      // 0.0 .. 4.0

// ---- Required operations ---------------------------------------------------

// Validates the new record, checks for a duplicate roll, and appends it.
// Returns true on success; prints the specific reason and returns false on
// any validation failure.
bool addStudent(const Student& s);

// Returns the student with this roll. On "not found" the returned Student has
// an empty roll field (check with result.roll.empty()).
Student searchByRoll(const std::string& roll);

// Returns every student whose name CONTAINS the given text (case-insensitive).
std::vector<Student> searchByName(const std::string& part);

// Updates a single column (any column except the roll) of one student, then
// rewrites the whole file. Returns false if the roll does not exist.
bool updateStudent(const std::string& roll, int colIndex, const std::string& newValue);

// Soft delete: flips status to "inactive" without removing the physical row.
bool softDelete(const std::string& roll);

// Returns all active students sorted by roll number using SELECTION SORT.
std::vector<Student> listActiveStudents();

// ---- Bonus feature: interactive "search as you type" -----------------------
// Reads one character at a time in a while loop; after every keystroke it
// reprints the active students whose roll OR name starts-with / contains what
// has been typed so far. Demonstrates prefix matching with substr + length.
void searchAsYouType();

#endif // STUDENT_OPS_H
