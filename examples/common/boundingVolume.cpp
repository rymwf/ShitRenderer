#include "boundingVolume.h"

float RaySphereIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &c, float r) {
    auto l = c - o;
    auto s = glm::dot(l, d);
    auto l2 = glm::dot(l, l);
    auto r2 = r * r;
    if (s < 0 && l2 > r2) return -1;
    auto m2 = l2 - s * s;
    if (m2 > r2) return -1;
    auto q = std::sqrt(r2 - m2);
    float t;
    if (l2 > r2)
        t = s - q;
    else
        t = s + q;
    return t;
}

float RayOBBIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &c, glm::vec3 const &h,
                      glm::mat3 const &m) {
    auto tmin = -FLT_MAX;
    auto tmax = FLT_MAX;
    auto p = c - o;
    float e, f, t1, t2;
    for (int i = 0; i < 3; ++i) {
        e = glm::dot(m[i], p);
        f = glm::dot(m[i], d);
        if (abs(f) > FLT_EPSILON) {
            t1 = (e + h[i]) / f;
            t2 = (e - h[i]) / f;
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax || tmax < 0) return -1;
        } else if (-e - h[i] > 0 || -e + h[i] < 0)
            return -1;
    }
    return tmin > 0 ? tmin : tmax;
}

float RayAABBIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &pmin, glm::vec3 const &pmax) {
    auto tmin = -FLT_MAX;
    auto tmax = FLT_MAX;
    auto c = (pmax + pmin) / glm::vec3(2);
    auto h = (pmax - pmin) / glm::vec3(2);
    auto p = c - o;
    float t1, t2;
    for (int i = 0; i < 3; ++i) {
        if (abs(d[i]) > FLT_EPSILON) {
            t1 = (p[i] + h[i]) / d[i];
            t2 = (p[i] - h[i]) / d[i];
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax || tmax < 0) return -1;
        } else if (-p[i] - h[i] > 0 || -p[i] + h[i] < 0)
            return -1;
    }
    return tmin > 0 ? tmin : tmax;
}

glm::vec3 RayTriIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &p0, glm::vec3 const &p1,
                          glm::vec3 const &p2) {
    auto e1 = p1 - p0;
    auto e2 = p2 - p0;
    auto q = glm::cross(d, e2);
    auto a = glm::dot(e1, q);
    if (a > -FLT_EPSILON && a < FLT_EPSILON) return {-1, -1, -1};
    auto s = o - p0;
    auto u = glm::dot(s, q) / a;
    if (u < 0.) return {-1, -1, -1};
    auto r = glm::cross(s, e1);
    auto v = glm::dot(d, r) / a;
    if (v < 0 || u + v > 1.0) return {-1, -1, -1};
    auto t = glm::dot(e2, r) / a;
    return glm::vec3{u, v, t};
}
bool PointInPolygon(glm::vec3 const &p, int vertexCount, glm::vec3 const *vertices) {
    bool ret = false;
    auto e0 = vertices[vertexCount - 1];
    auto y0 = (e0.y >= p.y);
    bool y1;
    for (int i = 0; i < vertexCount; ++i) {
        auto &&e1 = vertices[i];
        y1 = (e1.y >= p.y);
        if (y0 != y1 && (((e1.y - p.y) * (e0.x - e1.x) >= (e1.x - p.x) * (e0.y - e1.y)) == y1)) ret = !ret;
        y0 = y1;
        e0 = e1;
    }
    return ret;
}

/**
 * @brief
 *
 * @param bmin
 * @param bmax
 * @param planeN
 * @param planeP
 * @return int -1 outside, 1 inside, 0 intersecting
 */
int PlaneAABBIntersect(glm::vec3 const &bmin, glm::vec3 const &bmax, glm::vec3 const &planeN, glm::vec3 const &planeP) {
    return 0;
}

bool AABB_intersect(glm::vec3 const &pmin_A, glm::vec3 const &pmax_A, glm::vec3 const &pmin_B,
                    glm::vec3 const &pmax_B) {
    for (int i = 0; i < 3; ++i) {
        if (pmin_A[i] > pmax_B[i] || pmin_B[i] > pmax_A[i]) return false;
    }
    return true;
}

//====================
glm::vec3 AxisAlignedBox::getCenter() const { return (minValue + maxValue) / glm::vec3(2); }
glm::vec3 AxisAlignedBox::getSize() const { return maxValue - minValue; }
AxisAlignedBox::Corners AxisAlignedBox::getAllCorners() const {
    return AxisAlignedBox::Corners{
        getCorner(static_cast<AxisAlignedBox::CornerEnum>(0)), getCorner(static_cast<AxisAlignedBox::CornerEnum>(1)),
        getCorner(static_cast<AxisAlignedBox::CornerEnum>(2)), getCorner(static_cast<AxisAlignedBox::CornerEnum>(3)),
        getCorner(static_cast<AxisAlignedBox::CornerEnum>(4)), getCorner(static_cast<AxisAlignedBox::CornerEnum>(5)),
        getCorner(static_cast<AxisAlignedBox::CornerEnum>(6)), getCorner(static_cast<AxisAlignedBox::CornerEnum>(7))};
}
glm::vec3 AxisAlignedBox::getCorner(AxisAlignedBox::CornerEnum corner) const {
    switch (corner) {
        case AxisAlignedBox::CornerEnum::FAR_LEFT_BOTTOM:
            return minValue;
        case AxisAlignedBox::CornerEnum::FAR_LEFT_TOP:
            return glm::vec3(minValue.x, maxValue.y, minValue.z);
        case AxisAlignedBox::CornerEnum::FAR_RIGHT_TOP:
            return glm::vec3(maxValue.x, maxValue.y, minValue.z);
        case AxisAlignedBox::CornerEnum::FAR_RIGHT_BOTTOM:
            return glm::vec3(maxValue.x, minValue.y, minValue.z);
        case AxisAlignedBox::CornerEnum::NEAR_RIGHT_BOTTOM:
            return glm::vec3(maxValue.x, minValue.y, maxValue.z);
        case AxisAlignedBox::CornerEnum::NEAR_LEFT_BOTTOM:
            return glm::vec3(minValue.x, minValue.y, maxValue.z);
        case AxisAlignedBox::CornerEnum::NEAR_LEFT_TOP:
            return glm::vec3(minValue.x, maxValue.y, maxValue.z);
        case AxisAlignedBox::CornerEnum::NEAR_RIGHT_TOP:
            return maxValue;
        default:
            return glm::vec3();
    }
}

void AxisAlignedBox::merge(const glm::vec3 &point) {
    switch (extent) {
        case VOL_NONE:
            minValue = maxValue = point;
            extent = VOL_FINITE;
            break;
        case VOL_FINITE:
            minValue = glm::min(minValue, point);
            maxValue = glm::max(maxValue, point);
            break;
        case VOL_INFINITE:
            break;
    }
}
void AxisAlignedBox::merge(const AxisAlignedBox &aabbBox) {
    if (extent == VOL_INFINITE || aabbBox.extent == VOL_NONE)
        return;
    else if (extent == VOL_NONE)
        *this = aabbBox;
    else if (aabbBox.extent == VOL_INFINITE)
        extent = VOL_INFINITE;
    else {
        minValue = glm::min(minValue, aabbBox.minValue);
        maxValue = glm::max(minValue, aabbBox.maxValue);
    }
}
void AxisAlignedBox::transform(const glm::mat3x4 &matrix) {
    if (extent == VOL_INFINITE) return;
    glm::vec3 oldMin = minValue, oldMax = maxValue;
    glm::vec4 currentCorner = glm::vec4(oldMin, 1);
    extent = VOL_NONE;

    merge(matrix * currentCorner);

    // min,min,max
    currentCorner.z = oldMax.z;
    merge(matrix * currentCorner);

    // min max max
    currentCorner.y = oldMax.y;
    merge(matrix * currentCorner);

    // min max min
    currentCorner.z = oldMin.z;
    merge(matrix * currentCorner);

    // max max min
    currentCorner.x = oldMax.x;
    merge(matrix * currentCorner);

    // max max max
    currentCorner.z = oldMax.z;
    merge(matrix * currentCorner);

    // max min max
    currentCorner.y = oldMin.y;
    merge(matrix * currentCorner);

    // max min min
    currentCorner.z = oldMin.z;
    merge(matrix * currentCorner);
}
void AxisAlignedBox::scale(const glm::vec3 &s) {
    if (extent != VOL_FINITE) return;
    minValue *= s;
    maxValue *= s;
}