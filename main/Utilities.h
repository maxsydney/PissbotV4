#ifndef UTILITIES_H
#define UTILITIES_H

#include "PBCommon.h"
#include <vector>

class Utilities
{
    static constexpr const char* Name = "Utilities";

    public:

        // Math
        static PBRet polyVal(const std::vector<double>& coeffs, double val, double& result);
        static PBRet interpLinear(const std::vector<double>& x, const std::vector<double>& y, double xVal, double& yVal);
        static double bound(double val, double lowerLim, double upperLim);

        // Checkers
        static bool check(double val);
        static bool check(const std::vector<double>& val);

};

// A circular buffer that allows overwriting. If an element is inserted when the
// buffer is full, the oldest element will be deleted
class CircularBuffer
{
    public:
        CircularBuffer(size_t size)
            : _buff(size, 0.0), _size(size) {}

        // Insert a value into the buffer
        void insert(double val);

        // Retrieve a value from the buffer at index. If index is greater than the 
        // size of the buffer, value within the buffer at the wrapped index will be 
        // returned
        double at(size_t index) const;

        // Zero the buffer
        void clear(void);

        // Get buffer size
        size_t size(void) const { return _size; }

        // Buffer access. If index is greater than the size of the buffer, value
        // within the buffer at the wrapped index will be returned
        double& operator[] (size_t index);

    private:

        size_t _computeOffset(size_t index) const;
        std::vector<double> _buff {};
        size_t _curr = 0;
        size_t _size = 0;
};

#endif // UTILITIES_H