#pragma once
#include <cmath>

namespace te {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    float length() const { return std::sqrt(x*x + y*y + z*z); }
};

struct Mat4 {
    float data[16] = {0};

    static Mat4 identity();
    static Mat4 translation(const Vec3& pos);
    static Mat4 scale(const Vec3& s);
};

inline Mat4 Mat4::identity() {
    Mat4 m{};
    m.data[0] = 1.0f;
    m.data[5] = 1.0f;
    m.data[10] = 1.0f;
    m.data[15] = 1.0f;
    return m;
}

inline Mat4 Mat4::translation(const Vec3& pos) {
    Mat4 m = identity();
    m.data[12] = pos.x;
    m.data[13] = pos.y;
    m.data[14] = pos.z;
    return m;
}

inline Mat4 Mat4::scale(const Vec3& s) {
    Mat4 m{};
    m.data[0] = s.x;
    m.data[5] = s.y;
    m.data[10] = s.z;
    m.data[15] = 1.0f;
    return m;
}

} // namespace te
