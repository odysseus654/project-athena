//
//  AABox.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple axis aligned box class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AABox_h
#define hifi_AABox_h

#include <glm/glm.hpp>

#include <QDebug>

#include "BoxBase.h"
#include "GeometryUtil.h"
#include "StreamUtils.h"

class AACube;
class Extents;
class Transform;

class AABox {

public:
    AABox(const AACube& other);
    AABox(const Extents& other);
    AABox(const glm::dvec3& corner, double size);
    AABox(const glm::dvec3& corner, const glm::dvec3& dimensions);
    AABox();
    ~AABox() {};

    void setBox(const glm::dvec3& corner, const glm::vec3& scale);

    void setBox(const glm::dvec3& corner, float scale);
    glm::dvec3 getFarthestVertex(const glm::vec3& normal) const; // return vertex most parallel to normal
    glm::dvec3 getNearestVertex(const glm::vec3& normal) const; // return vertex most anti-parallel to normal

    const glm::dvec3& getCorner() const { return _corner; }
    const glm::vec3& getScale() const { return _scale; }
    const glm::vec3& getDimensions() const { return _scale; }
    float getLargestDimension() const { return glm::max(_scale.x, glm::max(_scale.y, _scale.z)); }

    glm::vec3 calcCenter() const;
    glm::dvec3 calcTopFarLeft() const { return _corner + glm::dvec3(_scale); }

    const glm::dvec3& getMinimum() const { return _corner; }
    glm::dvec3 getMaximum() const { return _corner + glm::dvec3(_scale); }

    glm::dvec3 getVertex(BoxVertex vertex) const;

    const glm::dvec3& getMinimumPoint() const { return _corner; }
    glm::dvec3 getMaximumPoint() const { return calcTopFarLeft(); }

    bool contains(const Triangle& triangle) const;
    bool contains(const glm::dvec3& point) const;
    bool contains(const AABox& otherBox) const;
    bool touches(const AABox& otherBox) const;

    bool contains(const AACube& otherCube) const;
    bool touches(const AACube& otherCube) const;

    bool expandedContains(const glm::dvec3& point, double expansion) const;
    bool expandedIntersectsSegment(const glm::dvec3& start, const glm::dvec3& end, double expansion) const;
    bool findRayIntersection(const glm::dvec3& origin, const glm::vec3& direction, const glm::vec3& invDirection, double& distance,
                             BoxFace& face, glm::vec3& surfaceNormal) const;
    bool findParabolaIntersection(const glm::dvec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration,
                                  double& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal) const;
    bool rayHitsBoundingSphere(const glm::vec3& origin, const glm::vec3& direction) const;
    bool parabolaPlaneIntersectsBoundingSphere(const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration, const glm::vec3& normal) const;
    bool touchesSphere(const glm::dvec3& center, double radius) const; // fast but may generate false positives
    bool touchesAAEllipsoid(const glm::dvec3& center, const glm::dvec3& radials) const;
    bool findSpherePenetration(const glm::dvec3& center, double radius, glm::vec3& penetration) const;
    bool findCapsulePenetration(const glm::dvec3& start, const glm::dvec3& end, double radius, glm::vec3& penetration) const;

    bool isNull() const { return _scale == glm::vec3(0.0f, 0.0f, 0.0f); }

    AABox clamp(const glm::dvec3& min, const glm::dvec3& max) const;
    AABox clamp(double min, double max) const;

    inline AABox& operator+=(const glm::dvec3& point) {
        bool valid = !isInvalid();
        glm::dvec3 maximum = glm::max(_corner + glm::dvec3(_scale), point);
        _corner = glm::min(_corner, point);
        if (valid) {
            _scale = maximum - _corner;
        }
        return (*this);
    }

    inline AABox& operator+=(const AABox& box) {
        if (!box.isInvalid()) {
            (*this) += box._corner;
            (*this) += box.calcTopFarLeft();
        }
        return (*this);
    }

    // Translate the AABox just moving the corner
    void translate(const glm::vec3& translation) { _corner += translation; }

    // Rotate the AABox around its frame origin
    // meaning rotating the corners of the AABox around the point {0,0,0} and reevaluating the min max
    void rotate(const glm::quat& rotation);

    /// Scale the AABox
    void scale(float scale);
    void scale(const glm::vec3& scale);

    /// make the AABox bigger (scale about it's center)
    void embiggen(float scale);
    void embiggen(const glm::vec3& scale);

    // Set a new scale for the Box, but keep it centered at its current location
    void setScaleStayCentered(const glm::vec3& scale);

    // Transform the extents with transform
    void transform(const Transform& transform);

    // Transform the extents with matrix
    void transform(const glm::mat4& matrix);

    static const glm::vec3 INFINITY_VECTOR;

    bool isInvalid() const { return _corner.x == std::numeric_limits<float>::infinity(); }

    void clear() { _corner = INFINITY_VECTOR; _scale = glm::vec3(0.0f); }

    typedef enum {
        topLeftNear,
        topLeftFar,
        topRightNear,
        topRightFar,
        bottomLeftNear,
        bottomLeftFar,
        bottomRightNear,
        bottomRightFar
    } OctreeChild;

    AABox getOctreeChild(OctreeChild child) const; // returns the AABox of the would be octree child of this AABox

    glm::vec4 getPlane(BoxFace face) const;

private:
    glm::dvec3 getClosestPointOnFace(const glm::dvec3& point, BoxFace face) const;
    glm::dvec3 getClosestPointOnFace(const glm::dvec4& origin, const glm::vec4& direction, BoxFace face) const;

    static BoxFace getOppositeFace(BoxFace face);

    void checkPossibleParabolicIntersection(float t, int i, float& minDistance,
        const glm::vec3& origin, const glm::vec3& velocity, const glm::vec3& acceleration, bool& hit) const;

    glm::dvec3 _corner;
    glm::vec3 _scale;
};

inline bool operator==(const AABox& a, const AABox& b) {
    return a.getCorner() == b.getCorner() && a.getDimensions() == b.getDimensions();
}

inline QDebug operator<<(QDebug debug, const AABox& box) {
    debug << "AABox[ ("
            << box.getCorner().x << "," << box.getCorner().y << "," << box.getCorner().z << " ) to ("
            << box.calcTopFarLeft().x << "," << box.calcTopFarLeft().y << "," << box.calcTopFarLeft().z << ") size: ("
            << box.getDimensions().x << "," << box.getDimensions().y << "," << box.getDimensions().z << ")"
            << "]";

    return debug;
}

#endif // hifi_AABox_h
