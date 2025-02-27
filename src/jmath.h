#pragma once
#include <cmath>
#include "SDL_rect.h"

class Vec2
{
public:
	double x;
	double y;

	Vec2() : x(0), y(0) {}
	Vec2(double x, double y) : x(x), y(y) {}

	Vec2 operator+(const Vec2& other) const
	{
		return Vec2(x + other.x, y + other.y);
	}

	Vec2 operator-(const Vec2& other) const
	{
		return Vec2(x - other.x, y - other.y);
	}

	Vec2 operator* (double scalar) const
	{
		return Vec2(x * scalar, y * scalar);
	}

	Vec2 operator/(double scalar) const
	{
		return Vec2(x / scalar, y / scalar);
	}

    void operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
    }

    void operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
    }

    void operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
    }

    void operator/=(double scalar) {
        x /= scalar;
        y /= scalar;
    }

	double magnitudeSquared() const
	{
		return x * x + y * y;
	}

	double magnitude() const
	{
		return sqrt(magnitudeSquared());
	}

	double Length() const {
		return magnitude();
	}

	Vec2 GetNormal()  {
		return Vec2(-y, x).Normalize();
	}

	double dot(const Vec2& other) const
	{
		return x * other.x + y * other.y;
	}

	Vec2 Normalize() const
	{
		double length = magnitude();
		return Vec2(x / length, y / length);
	}
	
	double cross(const Vec2& other) const
	{
		return x * other.y - y * other.x;
	}

	Vec2 reflect(const Vec2& normal) const
	{
		return Vec2(
			x - 2 * dot(normal) * normal.x, 
			y - 2 * dot(normal) * normal.y
		);
	}

    void ZeroOut() {
        x = 0;
        y = 0;
    }

    void Clamp(double min, double max) {
        if (x < min) x = min;
        if (x > max) x = max;
        if (y < min) y = min;
        if (y > max) y = max;
    }
};



class Mat3
{
public:
    double data[3 * 3];
    Mat3(double a, double b, double c, double d, double e, double f, double g, double h, double i)
        : data{a, b, c, d, e, f, g, h, i}
    {
    }

	Mat3() {
		data[0] = 1.0;
		data[1] = 0.0;
		data[2] = 0.0;
		data[3] = 0.0;
		data[4] = 1.0;
		data[5] = 0.0;
		data[6] = 0.0;
		data[7] = 0.0;
		data[8] = 1.0;
	}

    double* operator[](size_t idx)
    {
        return data + idx * 3;
    }

    const double* operator[](size_t idx) const
    {
        return data + idx * 3;
    }

    
    double& at(size_t row, size_t col)
    {
        return data[row * 3 + col];
    }

    const double& at(size_t row, size_t col) const
    {
        return data[row * 3 + col];
    }

    void set(size_t row, size_t col, double value)
    {
        data[row * 3 + col] = value;
    }

	
    void scale(double x, double y)
    {
        data[0] *= x;
        data[3] *= y;
        data[6] *= x;
        data[1] *= y;
        data[4] *= x;
        data[7] *= y;
    }

    void scale(double v)
    {
        scale(v, v);
    }

    Mat3 operator*(const Mat3& other) const
    {
        Mat3 result = *this;
        result *= other;
        return result;
    }

    Mat3& operator*=(const Mat3& other)
    {
        data[0] = other.data[0] * data[0] + other.data[3] * data[1] + other.data[6] * data[2];
        data[1] = other.data[1] * data[0] + other.data[4] * data[1] + other.data[7] * data[2];
        data[2] = other.data[2] * data[0] + other.data[5] * data[1] + other.data[8] * data[2];

        data[3] = other.data[0] * data[3] + other.data[3] * data[4] + other.data[6] * data[5];
        data[4] = other.data[1] * data[3] + other.data[4] * data[4] + other.data[7] * data[5];
        data[5] = other.data[2] * data[3] + other.data[5] * data[4] + other.data[8] * data[5];

        data[6] = other.data[0] * data[6] + other.data[3] * data[7] + other.data[6] * data[8];
        data[7] = other.data[1] * data[6] + other.data[4] * data[7] + other.data[7] * data[8];
        data[8] = other.data[2] * data[6] + other.data[5] * data[7] + other.data[8] * data[8];

        return *this;
    }

	
    Vec2 operator*(const Vec2& other) const
    {
        double x = data[0] * other.x + data[3] * other.y + data[6] * 1.0;
        double y = data[1] * other.x + data[4] * other.y + data[7] * 1.0;
        return Vec2(x, y);
    }
	

	static Mat3 Translate(double x, double y)
    {
        Mat3 res;
        res.set(2, 0, x);
        res.set(2, 1, y);
        return res;
    }

    static Mat3 Scale(double x, double y)
    {
        Mat3 res;
        res.set(0, 0, x);
        res.set(1, 1, y);
        return res;
    }

    static Mat3 Identity()
    {
        Mat3 result;
        return result;
    }
};

class Rect
{
public:
    // x,y is center of rect, w,h is full width and height
    int x;
    int y;
    int w;
    int h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    ~Rect() = default;

    Vec2 Center() { return Vec2(x, y); }

    inline int L() const { return (x - (w >> 1)); }
    inline int R() const { return (x + (w >> 1)); }
    inline int T() const { return (y + (h >> 1)); }
    inline int B() const { return (y - (h >> 1)); }
    inline int Ox() const { return (x); }
    inline int Oy() const { return (y); }
    inline int W2() const { return (w >> 1); }
    inline int H2() const { return (h >> 1); }
    inline int W4() const { return (w >> 2); }
    inline int H4() const { return (h >> 2); }
    inline Rect TR() { return Rect(x + W4(), y + H4(), W2(), H2()); }
    inline Rect TL() { return Rect(x - W4(), y + H4(), W2(), H2()); }
    inline Rect BL() { return Rect(x - W4(), y - H4(), W2(), H2()); }
    inline Rect BR() { return Rect(x + W4(), y - H4(), W2(), H2()); }

    bool Intersects(Rect r)
    {
        return (
            r.L() < R() &&
            r.R() > L() &&
            r.T() > B() &&
            r.B() < T());
    }

    SDL_Rect ToSDL(Mat3 &transform)
    {
        Vec2 p = transform * Vec2(L(), T());
        SDL_Rect rect;
        rect.x = (int)p.x;
        rect.y = (int)p.y;
        rect.w = w;
        rect.h = h;
        return rect;
    }
};