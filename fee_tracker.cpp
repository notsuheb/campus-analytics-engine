// ============================================================================
// M6 - fee_tracker.cpp
// ----------------------------------------------------------------------------
// Fee logic with fully manual date arithmetic.
// ============================================================================

#include "fee_tracker.h"
#include "filehandler.h"
#include "student_ops.h"
#include <iostream>
#include <iomanip>

using namespace std;

// ----------------------------------------------------------------------------
// Private date helpers.
// ----------------------------------------------------------------------------
static bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

// Days in each month for a given year (index 0 = January).
static int daysInMonth(int month, int year) {
    int table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && isLeapYear(year)) return 29;
    return table[month - 1];
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

// ----------------------------------------------------------------------------
// isValidDate: must be exactly DD-MM-YYYY with dashes in the right places,
// all digits, and sensible day/month ranges.
// ----------------------------------------------------------------------------
bool isValidDate(const string& date) {
    if (date.size() != 10) return false;
    if (date[2] != '-' || date[5] != '-') return false;

    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (!isDigit(date[i])) return false;
    }

    int day   = toInt(date.substr(0, 2));
    int month = toInt(date.substr(3, 2));
    int year  = toInt(date.substr(6, 4));

    if (month < 1 || month > 12) return false;
    if (year < 1) return false;
    if (day < 1 || day > daysInMonth(month, year)) return false;
    return true;
}

// ----------------------------------------------------------------------------
// dateToDays: convert a valid DD-MM-YYYY into a single running day number so
// that two dates can be compared by simple subtraction.
// ----------------------------------------------------------------------------
static long dateToDays(const string& date) {
    int day   = toInt(date.substr(0, 2));
    int month = toInt(date.substr(3, 2));
    int year  = toInt(date.substr(6, 4));

    long total = 0;
    // whole years before this one
    for (int y = 1; y < year; y++) {
        total += isLeapYear(y) ? 366 : 365;
    }
    // whole months before this one, in the current year
    for (int m = 1; m < month; m++) {
        total += daysInMonth(m, year);
    }
    total += day;
    return total;
}

// ----------------------------------------------------------------------------
// daysBetween: d2 - d1 (negative if d2 is earlier). Invalid dates count as 0.
// ----------------------------------------------------------------------------
int daysBetween(const string& d1, const string& d2) {
    if (!isValidDate(d1) || !isValidDate(d2)) return 0;
    return (int)(dateToDays(d2) - dateToDays(d1));
}

// ----------------------------------------------------------------------------
// computeLateFine: 2% of tuition per complete late week.
// Only meaningful when a real paid_date exists and is after the due date.
// ----------------------------------------------------------------------------
double computeLateFine(const string& roll) {
    Row fee = findRow(FEES_FILE, FEE_ROLL, roll);
    if (fee.empty()) return 0.0;

    string due  = fee[FEE_DUE];
    string paid = fee[FEE_PAYDATE];
    if (!isValidDate(paid)) return 0.0;        // "00-00-0000" -> no fine here

    int daysLate = daysBetween(due, paid);
    if (daysLate <= 0) return 0.0;             // paid on time or early

    int weeks = daysLate / 7;                  // complete weeks only
    double tuition = toDouble(fee[FEE_TOTAL]);
    return tuition * WEEKLY_FINE_RATE * weeks;
}

// ----------------------------------------------------------------------------
// recordPayment: validate, add the amount, restamp date/method/status.
// ----------------------------------------------------------------------------
bool recordPayment(const string& roll, double amount,
                   const string& date, const string& method) {
    if (!isValidDate(date)) {
        cout << "  [X] Date must be a valid DD-MM-YYYY.\n";
        return false;
    }
    if (amount <= 0) {
        cout << "  [X] Payment amount must be positive.\n";
        return false;
    }

    Table rows = readTXT(FEES_FILE);
    bool found = false;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][FEE_ROLL] == roll) {
            double total   = toDouble(rows[i][FEE_TOTAL]);
            double paidNow = toDouble(rows[i][FEE_PAID]) + amount;
            if (paidNow > total) paidNow = total;     // never over-pay

            rows[i][FEE_PAID]    = doubleToStr(paidNow, 0);
            rows[i][FEE_PAYDATE] = date;
            rows[i][FEE_METHOD]  = method;

            if (paidNow >= total) {
                bool late = daysBetween(rows[i][FEE_DUE], date) > 0;
                rows[i][FEE_STATUS] = late ? "paid_late" : "paid";
            } else {
                rows[i][FEE_STATUS] = "partial";
            }
            found = true;
            break;
        }
    }
    if (!found) {
        cout << "  [X] No fee record for roll " << roll << ".\n";
        return false;
    }

    writeTXT(FEES_FILE, readHeader(FEES_FILE), rows);
    cout << "  [OK] Payment of " << doubleToStr(amount, 0)
         << " recorded for " << roll << ".\n";
    return true;
}

// ----------------------------------------------------------------------------
// generateReceipt: aligned columns via setw/setfill.
// ----------------------------------------------------------------------------
void generateReceipt(const string& roll) {
    Row fee = findRow(FEES_FILE, FEE_ROLL, roll);
    if (fee.empty()) {
        cout << "  [X] No fee record for roll " << roll << ".\n";
        return;
    }
    Student s = searchByRoll(roll);
    string name = s.roll.empty() ? "(unknown)" : s.name;

    double tuition = toDouble(fee[FEE_TOTAL]);
    double paid    = toDouble(fee[FEE_PAID]);
    double fine    = computeLateFine(roll);
    double balance = tuition + fine - paid;

    cout << "\n";
    cout << "  +" << string(40, '-') << "+\n";
    cout << "  |" << setw(40) << left << "  FEE RECEIPT" << "|\n";
    cout << "  +" << string(40, '-') << "+\n";
    cout << "  | Roll  : " << setw(31) << left << roll << "|\n";
    cout << "  | Name  : " << setw(31) << left << name << "|\n";
    cout << "  +" << string(40, '-') << "+\n";

    cout << right << setfill(' ');
    cout << "  | Tuition" << setw(31) << ("Rs " + doubleToStr(tuition, 0)) << " |\n";
    cout << "  | Late fine" << setw(29) << ("Rs " + doubleToStr(fine, 0)) << " |\n";
    cout << "  | Paid" << setw(34) << ("Rs " + doubleToStr(paid, 0)) << " |\n";
    cout << "  +" << string(40, '-') << "+\n";
    cout << "  | Balance" << setw(31) << ("Rs " + doubleToStr(balance, 0)) << " |\n";
    cout << "  +" << string(40, '-') << "+\n";
}

// ----------------------------------------------------------------------------
// getDefaulters: collect everyone with an outstanding balance, then BUBBLE
// SORT them so the largest debt is first.
// ----------------------------------------------------------------------------
vector<Defaulter> getDefaulters() {
    vector<Defaulter> list;
    Table rows = readTXT(FEES_FILE);

    for (size_t i = 0; i < rows.size(); i++) {
        double total = toDouble(rows[i][FEE_TOTAL]);
        double paid  = toDouble(rows[i][FEE_PAID]);
        double owing = total - paid;

        // Defaulter = still owes money AND the due date has already passed.
        if (owing > 0 && daysBetween(rows[i][FEE_DUE], SYSTEM_DATE) > 0) {
            Defaulter d;
            d.roll        = rows[i][FEE_ROLL];
            d.outstanding = owing;
            d.weeksOverdue = daysBetween(rows[i][FEE_DUE], SYSTEM_DATE) / 7;
            Student s     = searchByRoll(d.roll);
            d.name        = s.roll.empty() ? "(unknown)" : s.name;
            list.push_back(d);
        }
    }

    // Bubble sort: repeatedly swap neighbours that are out of order.
    for (size_t i = 0; i + 1 < list.size(); i++) {
        for (size_t j = 0; j + 1 < list.size() - i; j++) {
            if (list[j].outstanding < list[j + 1].outstanding) {
                Defaulter temp = list[j];
                list[j]        = list[j + 1];
                list[j + 1]    = temp;
            }
        }
    }
    return list;
}
