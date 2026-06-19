# ============================================================================
# Makefile - Campus Analytics Engine
# ----------------------------------------------------------------------------
#   make         build the campus_engine executable
#   make run     build (if needed) and run the program
#   make clean   remove the binary, object files and exported reports
# ============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
TARGET   := campus_engine

# Every .cpp file in the project (M0 through M7).
SOURCES  := main.cpp filehandler.cpp student_ops.cpp course_ops.cpp \
            attendance.cpp grades.cpp fee_tracker.cpp reports.cpp

# Build the executable from all sources in one step.
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

# Convenience target: build then launch.
run: $(TARGET)
	./$(TARGET)

# Remove build products and any reports generated at runtime.
clean:
	rm -f $(TARGET) *.o *_report.txt merit.txt

.PHONY: run clean
