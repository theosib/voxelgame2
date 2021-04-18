#ifndef __INCLUDED_SPLINE_HPP
#define __INCLUDED_SPLINE_HPP

#include <stdio.h>
#include <math.h>

struct Spline {
    double p0, p1, p2;
    double v0, v2;
    double a0, b0, c0, d0;
    double a1, b1, c1, d1;
    
    void compute_coefficients();
    double compute(double x);
};

struct Spline2D {
    double z00, z10, z20;
    double z01, z11, z21;
    double z02, z12, z22;
    double vx0dy0, vx1dy0, vx2dy0;
    double vx0dy2, vx1dy2, vx2dy2;
    double vy0dx0, vy1dx0, vy2dx0;
    double vy0dx2, vy1dx2, vy2dx2;
    
    Spline x0, x1, x2;
    Spline y0, y1, y2;
    
    Spline2D();
    void compute_coefficients();
    double compute(double x, double y);
};

#endif