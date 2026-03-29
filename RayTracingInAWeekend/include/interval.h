#ifndef INTERVAL_H
#define INTERVAL_H

/**
 * A class representing an interval on the real line.
 */
class interval {
  public:
    double min, max;

    interval() : min(+infinity), max(-infinity) {} // Default interval is empty

    interval(double min, double max) : min(min), max(max) {}

    /**
     * Returns the size of the interval (max - min).
     */
    double size() const
    {
        return max - min;
    }

    /**
     * Checks if the interval contains a point x (inclusive).
     */
    bool contains(double x) const {
        return min <= x && x <= max;
    }

    /**
     * Checks if the interval strictly surrounds a point x.
     */
    bool surrounds(double x) const {
        return min < x && x < max;
    }

    /**
     * Clamps a value x to be within the interval [min, max].
     */
    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const interval empty, universe;
};

const interval interval::empty    = interval(+infinity, -infinity); // Empty interval has min > max, so it contains no points.
const interval interval::universe = interval(-infinity, +infinity); // Universe interval contains all real numbers.
#endif