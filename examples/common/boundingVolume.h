#pragma once
#include <array>
#include <optional>

#include "glm/glm.hpp"

float RaySphereIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &c, float r);

/**
 * @brief
 *
 * @param o
 * @param d
 * @param c
 * @param h
 * @param m m is OBB box transform matrix
 * @return float -1 mean no intersection
 */
float RayOBBIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &c, glm::vec3 const &h,
                      glm::mat3 const &m);

float RayAABBIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &pmin, glm::vec3 const &pmax);

/**
 * @brief
 *
 * @param o
 * @param d
 * @param p0
 * @param p1
 * @param p2
 * @return glm::vec3 {u,v,t}, {-1,-1,-1} mean no intersection
 */
glm::vec3 RayTriIntersect(glm::vec3 const &o, glm::vec3 const &d, glm::vec3 const &p0, glm::vec3 const &p1,
                          glm::vec3 const &p2);

bool PointInPolygon(glm::vec3 const &p, int vertexCount, glm::vec3 const *vertices);

bool AABB_intersect(glm::vec3 const &pmin_A, glm::vec3 const &pmax_A, glm::vec3 const &pmin_B, glm::vec3 const &pmax_B);

struct AxisAlignedBox {
    glm::vec3 minValue{FLT_MAX};
    glm::vec3 maxValue{-FLT_MAX};

    /*
       1-------2
      /|      /|
     / |     / |
    5-------4  |
    |  0----|--3
    | /     | /
    |/      |/
    6-------7
    */
    enum class CornerEnum {
        FAR_LEFT_BOTTOM,
        FAR_LEFT_TOP,
        FAR_RIGHT_TOP,
        FAR_RIGHT_BOTTOM,
        NEAR_RIGHT_BOTTOM,
        NEAR_LEFT_BOTTOM,
        NEAR_LEFT_TOP,
        NEAR_RIGHT_TOP
    };
    typedef std::array<glm::vec3, 8> Corners;

    enum Volume {
        VOL_NONE,  // empty box
        VOL_FINITE,
        VOL_INFINITE  //
    };
    Volume extent{VOL_NONE};

    glm::vec3 getCenter() const;
    glm::vec3 getSize() const;
    Corners getAllCorners() const;
    glm::vec3 getCorner(CornerEnum corner) const;

    void merge(const glm::vec3 &point);
    void merge(const AxisAlignedBox &aabbBox);
    void transform(const glm::mat3x4 &matrix);

    void scale(const glm::vec3 &s);
};
struct BoundingVolume {
    struct Box {
        using AABB = AxisAlignedBox;
        AABB aabb;
        struct OBB {
        } obb;
    } box;
    struct Sphere {
    } sphere;
    struct Ellipsoid {
    } ellipsoid;
    struct Cylinder {
    } cylinder;
};