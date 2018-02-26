#ifndef Magnum_Examples_Octree_h
#define Magnum_Examples_Octree_h
/*
    This file is part of Magnum.
    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018
              Vladimír Vondruš <mosra@centrum.cz>
    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/
#include <algorithm> 
#include <vector>

#include "RaytracingTypes.h"

namespace Magnum { namespace Examples {

struct BoundingBox {
    Vector3 _size;
    Vector3 _minPoint, _maxPoint;

    BoundingBox() {}

    BoundingBox(const Vector3& _minPoint, const Vector3& _maxPoint) : _minPoint(_minPoint), _maxPoint(_maxPoint) {
        _size = _maxPoint - _minPoint;
    }

    BoundingBox(const Vector3& size) : _size(size) {
        _maxPoint = _size / 2.f;
        _minPoint = -_maxPoint;
    }

    bool contains(const BoundingBox& other) {
        for(UnsignedInt i = 0; i < 3; i++) {
            if(other._maxPoint[i] > _maxPoint[i] || other._maxPoint[i] < _minPoint[i])
                return false;
            if(other._minPoint[i] > _maxPoint[i] || other._minPoint[i] < _minPoint[i])
                return false;
        }
        return true;
    }

    bool contains(const Triangle& tri) {
        for(UnsignedInt i = 0; i < 3; i++) {
            if(tri.A[i] > _maxPoint[i] || tri.A[i] < _minPoint[i])
                return false;
            if(tri.B[i] > _maxPoint[i] || tri.B[i] < _minPoint[i])
                return false;
            if(tri.C[i] > _maxPoint[i] || tri.C[i] < _minPoint[i])
                return false;
        }
        return true;
    }
};

class OctreeNode {
    public:
        OctreeNode(const Vector3& size, OctreeNode* parent = nullptr) : _bounds(size), _parent(parent) {}
        OctreeNode(const BoundingBox& bbox, OctreeNode* parent = nullptr) : _bounds(bbox), _parent(parent) {}
        /* @todo: nice desctructor */

        OctreeNode* parent() { return _parent; }
        OctreeNode* child(UnsignedInt i) { return _children[i]; }
        std::vector<Object*> objects() { return _objects; }

        unsigned char activeNodes() { return _activeNodes; }

        OctreeNode& update() {
            if(!_treeBuilt)
                buildTree();

            return *this;
        }

        OctreeNode& insertObject(Object* object) {
            /* if the tree is not built, just add it to the list of objects */
            if(!_treeBuilt) {
                _objects.push_back(object);
                return *this;
            }

            /* if leaf node and no active child */
            if(_objects.size() < _minObjects && !_activeNodes) {
                _objects.push_back(object);
                return *this;
            }

            Vector3 size = _bounds._size;

            /* if too small, return */
            if(size[0] <= _minSize || size[1] <= _minSize || size[2] <= _minSize) {
                _objects.push_back(object);
                return *this;
            }

            /* try to insert deeper */
            Vector3 half = size / 2.0f;
            Vector3 center = _bounds._minPoint + half;

            /* bounding boxes for children */
            std::vector<BoundingBox> octant(8);
            octant[0] = (_children[0]) ? _children[0]->_bounds : BoundingBox(_bounds._minPoint, center);
            octant[1] = (_children[1]) ? _children[1]->_bounds : BoundingBox({center.x(), _bounds._minPoint.y(), _bounds._minPoint.z()}, {_bounds._maxPoint.x(), center.y(), center.z()});
            octant[2] = (_children[2]) ? _children[2]->_bounds : BoundingBox({center.x(), _bounds._minPoint.y(), center.z()}, {_bounds._maxPoint.x(), center.y(), _bounds._maxPoint.z()});
            octant[3] = (_children[3]) ? _children[3]->_bounds : BoundingBox({_bounds._minPoint.x(), _bounds._minPoint.y(), center.z()}, {center.x(), center.y(), _bounds._maxPoint.z()});
            octant[4] = (_children[4]) ? _children[4]->_bounds : BoundingBox({_bounds._minPoint.x(), center.y(), _bounds._minPoint.z()}, {center.x(), _bounds._maxPoint.y(), center.z()});
            octant[5] = (_children[5]) ? _children[5]->_bounds : BoundingBox({center.x(), center.y(), _bounds._minPoint.z()}, {_bounds._maxPoint.x(), _bounds._maxPoint.y(), center.z()});
            octant[6] = (_children[6]) ? _children[6]->_bounds : BoundingBox(center, _bounds._maxPoint);
            octant[7] = (_children[7]) ? _children[7]->_bounds : BoundingBox({_bounds._minPoint.x(), center.y(), center.z()}, {center.x(), _bounds._maxPoint.y(), _bounds._maxPoint.z()});

            if(_bounds.contains(object->triangle)) {
                bool found = false;
                for(UnsignedInt i = 0; i < 8; i++) {
                    if(octant[i].contains(object->triangle)) {
                        if(!_children[i]) {
                            _children[i] = new OctreeNode(octant[i], this);
                            _activeNodes |= (1 << i);
                        }
                        _children[i]->insertObject(object);
                        found = true;
                        break;
                    }
                }
                /* if not inserted in a child, means that intersects at least 2 of them */
                if(!found)
                    _objects.push_back(object);
            }
            else
                CORRADE_ASSERT(false, "Octree insert: We should never reach this point!", *this);
            // /* if the object not in the region, we need to re-build the tree */
            // else
            //     buildTree();
            return *this;
        }
    private:
        /* Helper variables */
        BoundingBox _bounds;
        bool _treeBuilt = false;
        Float _minSize = 0.25f;
        UnsignedInt _minObjects = 2;
        unsigned char _activeNodes = 0;

        /* Actual things */
        OctreeNode* _children[8];
        OctreeNode* _parent = nullptr;
        std::vector<Object*> _objects;//, _pendingObjects;

        OctreeNode& buildTree() {
            /* if just one object or empty, return */
            if(_objects.size() <= _minObjects)
                return *this;

            Vector3 size = _bounds._size;

            /* if too small, return */
            if(size[0] <= _minSize || size[1] <= _minSize || size[2] <= _minSize)
                return *this;

            /* need to split */
            Vector3 half = size / 2.0f;
            Vector3 center = _bounds._minPoint + half;

            /* create bounding boxes for children */
            std::vector<BoundingBox> octant(8);
            octant[0] = BoundingBox(_bounds._minPoint, center);
            octant[1] = BoundingBox({center.x(), _bounds._minPoint.y(), _bounds._minPoint.z()}, {_bounds._maxPoint.x(), center.y(), center.z()});
            octant[2] = BoundingBox({center.x(), _bounds._minPoint.y(), center.z()}, {_bounds._maxPoint.x(), center.y(), _bounds._maxPoint.z()});
            octant[3] = BoundingBox({_bounds._minPoint.x(), _bounds._minPoint.y(), center.z()}, {center.x(), center.y(), _bounds._maxPoint.z()});
            octant[4] = BoundingBox({_bounds._minPoint.x(), center.y(), _bounds._minPoint.z()}, {center.x(), _bounds._maxPoint.y(), center.z()});
            octant[5] = BoundingBox({center.x(), center.y(), _bounds._minPoint.z()}, {_bounds._maxPoint.x(), _bounds._maxPoint.y(), center.z()});
            octant[6] = BoundingBox(center, _bounds._maxPoint);
            octant[7] = BoundingBox({_bounds._minPoint.x(), center.y(), center.z()}, {center.x(), _bounds._maxPoint.y(), _bounds._maxPoint.z()});

            /* create the children nodes */
            for(UnsignedInt i = 0; i < 8; i++) _children[i] = new OctreeNode(octant[i], this);

            std::vector<Object*> toRemove;

            for(Object* obj : _objects) {
                for(UnsignedInt i = 0; i < 8; i++) {
                    if(octant[i].contains(obj->triangle)) {
                        _children[i]->insertObject(obj);
                        toRemove.emplace_back(obj);
                        break;
                    }
                }
            }

            /* remove moved objects */
            /* @todo: make it better than this */
            for(Object* obj : toRemove)
                _objects.erase(std::remove(_objects.begin(), _objects.end(), obj));

            /* build the children trees */
            for(UnsignedInt i = 0; i < 8; i++) {
                if(_children[i]->objects().size() > 0) {
                    _children[i]->buildTree();
                    _activeNodes |= (1 << i);
                }
            }

            /* mark tree as built */
            _treeBuilt = true;

            return *this;
        }
};

}}

#endif