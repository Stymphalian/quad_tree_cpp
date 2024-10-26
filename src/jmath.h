#pragma once
#include <cmath>

class Vector2
{
public:
	float x;
	float y;

	Vector2() : x(0), y(0) {}
	Vector2(float x, float y) : x(x), y(y) {}

	Vector2 operator+(const Vector2& other) const
	{
		return Vector2(x + other.x, y + other.y);
	}

	Vector2 operator-(const Vector2& other) const
	{
		return Vector2(x - other.x, y - other.y);
	}

	Vector2 operator* (float scalar) const
	{
		return Vector2(x * scalar, y * scalar);
	}

	Vector2 operator/(float scalar) const
	{
		return Vector2(x / scalar, y / scalar);
	}

	float magnitudeSquared() const
	{
		return x * x + y * y;
	}

	float magnitude() const
	{
		return sqrt(magnitudeSquared());
	}

	float dot(const Vector2& other) const
	{
		return x * other.x + y * other.y;
	}

	Vector2 normalize() const
	{
		float length = magnitude();
		return Vector2(x / length, y / length);
	}
	
	float cross(const Vector2& other) const
	{
		return x * other.y - y * other.x;
	}

	Vector2 reflect(const Vector2& normal) const
	{
		return Vector2(
			x - 2 * dot(normal) * normal.x, 
			y - 2 * dot(normal) * normal.y
		);
	}
};
