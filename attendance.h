#ifndef ATTENDANCE_H
#define ATTENDANCE_H

// ============================================================================
// M4 - attendance.h
// ----------------------------------------------------------------------------
// Marking and analysing attendance (Present / Absent / Late).
// Depends on M1 (filehandler) and M3 (course_ops, to know who is enrolled).
// ============================================================================

#include <string>
#include <vector>
#include "filehandler.h"

// ---- Column positions inside attendance_log.txt ----------------------------
const int ATT_LOGID  = 0;
const int ATT_ROLL   = 1;
const int ATT_CODE   = 2;
const int ATT_DATE   = 3;
const int ATT_STATUS = 4;   // "P", "A" or "L"

// One row of the shortage report: a student who is below 75% in one course.
struct ShortageRecord {
    std::string roll;
    std::string name;
    std::string course;
    double      percentage;
};

// ---- Required operations ---------------------------------------------------

// Marks attendance for every active student enrolled in a course on a date.
// Before writing, it snapshots the whole attendance table so undoLastSession
// can roll the change back. Each student is prompted for P/A/L.
void markAttendance(const std::string& courseCode, const std::string& date);

// (present + 0.5*late) / totalSessions * 100, accumulated with a loop.
// Returns 0.0 when the student has no recorded sessions for the course.
double getAttendancePct(const std::string& roll, const std::string& courseCode);

// Every (student, course) pair whose attendance is below 75%.
std::vector<ShortageRecord> getShortageList();

// Restores the attendance file from the pre-session snapshot taken by
// markAttendance. Returns false if no snapshot exists yet.
bool undoLastSession();

// Prints a neat table of every enrolled student's status on a given date.
void printDailySheet(const std::string& courseCode, const std::string& date);

#endif // ATTENDANCE_H
