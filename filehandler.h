#ifndef FILEHANDLER_H
#define FILEHANDLER_H

// ============================================================================
// M1 - filehandler.h
// ----------------------------------------------------------------------------
// The lowest-level module. It knows NOTHING about students, courses or fees.
// Its only job is to turn a comma-separated text file into rows of strings
// (and back again). Every other module calls these functions instead of
// touching ifstream/ofstream directly. This is the "standalone" module in the
// dependency table of the project PDF.
// ============================================================================

#include <string>
#include <vector>

// A single line of a data file, already split into its individual fields.
// Example: the line "BSAI-23-001,Ahmed Raza,..." becomes
//          Row = { "BSAI-23-001", "Ahmed Raza", ... }
typedef std::vector<std::string> Row;

// A whole file (minus its header) is just a list of rows.
typedef std::vector<Row> Table;

// ---- File paths (no hard-coded data, only the file LOCATIONS live here) -----
// Centralising the paths means the rest of the program never spells a filename
// out by hand, so renaming a file is a one-line change.
const std::string DATA_DIR          = "data/";
const std::string STUDENTS_FILE     = DATA_DIR + "students.txt";
const std::string COURSES_FILE      = DATA_DIR + "courses.txt";
const std::string ENROLLMENTS_FILE  = DATA_DIR + "enrollments.txt";
const std::string ATTENDANCE_FILE   = DATA_DIR + "attendance_log.txt";
const std::string FEES_FILE         = DATA_DIR + "fees.txt";
const std::string GRADES_FILE       = DATA_DIR + "grades.txt";

// ---- Core file operations (required by the PDF) ----------------------------

// Opens the file, throws away the header line, and parses every remaining line
// one character at a time into a Row. Handles quoted fields that contain commas
// (e.g. "Khan, Ahmed") and ignores stray carriage-returns from Windows files.
Table readTXT(const std::string& path);

// Reads ONLY the first line of a file (the column header) so that a module can
// rewrite the file later without hard-coding the header text.
std::string readHeader(const std::string& path);

// Overwrites the file completely: writes the header line, then every row.
// Any field that itself contains a comma is wrapped in double quotes.
void writeTXT(const std::string& path, const std::string& header, const Table& rows);

// Adds ONE row to the end of the file without loading the whole file first.
void appendTXT(const std::string& path, const Row& row);

// Linear search: returns the first row whose field at colIndex equals value,
// or an empty Row if nothing matches.
Row findRow(const std::string& path, int colIndex, const std::string& value);

// Returns true if findRow would have found something.
bool rowExists(const std::string& path, int colIndex, const std::string& value);

// ---- Small shared utilities (used all over the project) --------------------

// Safe string -> number conversions that return 0 instead of crashing on junk.
int    toInt(const std::string& s);
double toDouble(const std::string& s);

// number -> string, kept here so no module needs its own copy.
std::string intToStr(int value);
std::string doubleToStr(double value, int decimals = 2);

#endif // FILEHANDLER_H
