#pragma once

#ifndef _VECTORMATH_HPP
#define _VECTORMATH_HPP

#include <cmath>

struct Vec3
{
public:
	double x, y, z;

	~Vec3() = default;

	constexpr Vec3(double _x, double _y, double _z)
		:x(_x), y(_y), z(_z)
	{ }

	constexpr Vec3(double v)
		: x(v), y(v), z(v)
	{ }

	constexpr Vec3()
		:x(.0), y(.0), z(.0)
	{ }

	constexpr Vec3& operator=(const Vec3&) = default;
	constexpr Vec3& operator=(const double b) {
		x, y, z = b;
		return *this;
	}

	constexpr Vec3 operator+(const Vec3& a) const
	{
		return Vec3(x + a.x,
					y + a.y,
					z + a.z);
	}

	constexpr Vec3 operator-(const Vec3& a) const
	{
		return Vec3(x - a.x,
					y - a.y,
					z - a.z);
	}

	constexpr Vec3 operator*(const Vec3& a) const
	{
		return Vec3(x * a.x,
					y * a.y,
					z * a.z);
	}

	constexpr Vec3 operator/(const Vec3& a) const
	{
		return Vec3(x / a.x,
					y / a.y,
					z / a.z);
	}

	friend constexpr Vec3 operator*(const double a, const Vec3& b) {
		return Vec3(Vec3(a) * b);
	}

	friend constexpr Vec3 operator*(const Vec3& a, const double b) {
		return Vec3(a * Vec3(b));
	}

	constexpr Vec3 operator/(const double b) const {
		return Vec3(*this / Vec3(b));
	}

	constexpr bool operator==(const Vec3& b) const {
		return ((x == b.x) && (y == b.x) && (z == b.z));
	}

	constexpr bool operator!=(const Vec3& b) const {
		return ((x != b.x) || (y != b.x) || (z != b.z));
	}

	constexpr double LengthSquared() const {
		return ((x * x) + (y * y) + (z * z));
	}

	inline double Length() const {
		return sqrt(LengthSquared());
	}

	inline Vec3 Normalized() const
	{
		const double len = Length();
		if (len == .0f)
			return { .0, .0, .0 };

		const double mag = 1.0 / len;

		return Vec3(x * mag,
					y * mag,
					z * mag);
	}

	constexpr static Vec3 CrossProduct(const Vec3& a, const Vec3& b)
	{
		return Vec3((a.y * b.z) - (a.z * b.y),
					(a.z * b.x) - (a.x * b.z),
					(a.x * b.y) - (a.y * b.x));
	}

	constexpr double DotProduct(const Vec3& b) const
	{
		return ((x * b.x) + (y * b.y) + (z * b.z));
	}
};

inline const Vec3 GetNormalFromPlane(const Vec3& p0, const Vec3& p1, const Vec3& p2)
{
	const Vec3 v1 = p0 - p1;
	const Vec3 v2 = p2 - p1;
	const Vec3 normal = Vec3::CrossProduct(v1, v2).Normalized();

	return normal;
}

#endif // !_VECTORMATH_HPP
