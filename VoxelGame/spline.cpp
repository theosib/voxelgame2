#include "spline.hpp"
#include <algorithm>

void Spline::compute_coefficients()
{
    a0 = p0;
    b0 = v0;
    c0 = -9.0*p0 + 12.0*p1 - 3.0*p2 - 3.5*v0 + 0.5*v2;
    d0 = 10.0*p0 - 16.0*p1 + 6.0*p2 + 3.0*v0 - v2;
    a1 = p1;
    b1 = -1.5*p0 + 1.5*p2 - 0.25*v0 - 0.25*v2;
    c1 = 6.0*p0 - 12*p1 + 6.0*p2 + v0 - v2;
    d1 = -6.0*p0 + 16.0*p1 - 10.0*p2 - v0 + 3.0*v2;
}

double Spline::compute(double x)
{
    if (x<0.5) {
        double xx = x*x;
        double xxx = xx*x;
        return a0 + b0*x + c0*xx + d0*xxx;
    } else {
        double x1 = (x-0.5);
        double xx = x1*x1;
        double xxx = xx*x1;
        return a1 + b1*x1 + c1*xx + d1*xxx;
    } 
}

void Spline2D::compute_coefficients()
{
    y0.v0 = vy0dx0;
    y0.v2 = vy0dx2;
    y1.v0 = vy1dx0;
    y1.v2 = vy1dx2;
    y2.v0 = vy2dx0;
    y2.v2 = vy2dx2;
    
    x0.v0 = vx0dy0;
    x0.v2 = vx0dy2;
    x1.v0 = vx1dy0;
    x1.v2 = vx1dy2;
    x2.v0 = vx2dy0;
    x2.v2 = vx2dy2;
    
    y0.p0 = z00;
    y0.p1 = z10;
    y0.p2 = z20;
    y1.p0 = z01;
    y1.p1 = z11;
    y1.p2 = z21;
    y2.p0 = z02;
    y2.p1 = z12;
    y2.p2 = z22;
    
    x0.p0 = z00;
    x0.p1 = z01;
    x0.p2 = z02;
    x1.p0 = z10;
    x1.p1 = z11;
    x1.p2 = z12;
    x2.p0 = z20;
    x2.p1 = z21;
    x2.p2 = z22;
    
    x0.compute_coefficients();
    x1.compute_coefficients();
    x2.compute_coefficients();
    y0.compute_coefficients();
    y1.compute_coefficients();
    y2.compute_coefficients();
}

double Spline2D::compute(double x, double y)
{
    double zx, zy;
    
    if (x < 0.5) {
        double z0 = x0.compute(y);
        double z1 = x1.compute(y);
        double t = 2*x;
        zx = z0*(1-t) + z1*t;
    } else {
        double z1 = x1.compute(y);
        double z2 = x2.compute(y);
        double t = 2*(x-0.5);
        zx = z1*(1-t) + z2*t;
    }
    
    if (y < 0.5) {
        double z0 = y0.compute(x);
        double z1 = y1.compute(x);
        double t = 2*y;
        zy = z0*(1-t) + z1*t;
    } else {
        double z1 = y1.compute(x);
        double z2 = y2.compute(x);
        double t = 2*(y-0.5);
        zy = z1*(1-t) + z2*t;
    }
    
    if (zx<0) zx = 0;
    if (zy<0) zy = 0;
    float z = sqrt(zx * zy);
    // float z = std::max(zx, zy);
    if (z > 1) z = 1;
    return z;
    // return sqrt(sqrt(zx * zy));
    // return (zx + zy) * 0.5;
}

Spline2D::Spline2D()
{
    z00 = 0;
    z10 = 0;
    z20 = 0;
    z01 = 0;
    z11 = 0;
    z21 = 0;
    z02 = 0;
    z12 = 0;
    z22 = 0;
    vx0dy0 = 0;
    vx1dy0 = 0;
    vx2dy0 = 0;
    vx0dy2 = 0;
    vx1dy2 = 0;
    vx2dy2 = 0;
    vy0dx0 = 0;
    vy1dx0 = 0;
    vy2dx0 = 0;
    vy0dx2 = 0;
    vy1dx2 = 0;
    vy2dx2 = 0;
}


#if 0
void test_1d()
{
    Spline s;
    s.p0 = 0;
    s.p1 = 1;
    s.p2 = 0;
    s.v0 = 1;
    s.v2 = -1;
    s.compute_coefficients();
    
    for (double x=0; x<=1; x+=0.0625) {
        double y = s.compute(x);
        printf("%5.2f %5.2f\n", x, y);
    }
}

void test_2d()
{
    Spline2D s;
    s.z00 = 0;
    s.z01 = 0;
    s.z02 = 0;
    s.z10 = 0;
    s.z11 = 1;
    s.z12 = 0;
    s.z20 = 0;
    s.z21 = 0;
    s.z22 = 0;
    
    // s.vx0dy0 = 1;
    // s.vx1dy0 = 1;
    // s.vx2dy0 = 1;
    // s.vx0dy2 = -1;
    // s.vx1dy2 = -1;
    // s.vx2dy2 = -1;
    // s.vy0dx0 = 1;
    // s.vy1dx0 = 1;
    // s.vy2dx0 = 1;
    // s.vy0dx2 = -1;
    // s.vy1dx2 = -1;
    // s.vy2dx2 = -1;
    
    s.vx0dy0 = 0;
    s.vx1dy0 = 0;
    s.vx2dy0 = 0;
    s.vx0dy2 = 0;
    s.vx1dy2 = 0;
    s.vx2dy2 = 0;
    s.vy0dx0 = 0;
    s.vy1dx0 = 0;
    s.vy2dx0 = 0;
    s.vy0dx2 = 0;
    s.vy1dx2 = 0;
    s.vy2dx2 = 0;
    
    s.compute_coefficients();
    
    for (double y=0; y<=1; y+=0.0625) {
        for (double x=0; x<=1; x+=0.0625) {
            double z = s.compute(x, y);
            printf("%5.2f\t%5.2f\t%5.2f\n", x, y, z);
            //printf("%5.2f ", z);
        }
        //printf("\n");
    }
}

int main()
{
    test_2d();
    return 0;
}
#endif