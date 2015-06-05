/*
 *
 *    Copyright (c) 2014
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */
#include "unittest.h"
#include "../include/interpolation.h"

#include <vector>

TEST(interpolate_linear) {
    std::vector<double> x = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<double> y = {1, 2, 0, 0, 0, 0, 0, 8, 9};
    InterpolateData f(x, y);
    COMPARE(f(1.5), 1.5);
    COMPARE(f(0), 0.0);
    COMPARE(f(10), 10.0);
}
