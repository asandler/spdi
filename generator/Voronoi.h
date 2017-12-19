#pragma once

#ifndef Voronoi_h
#define Voronoi_h

#include <list>
#include <queue>
#include <set>

#include "VPoint.h"
#include "VEdge.h"
#include "VParabola.h"
#include "VEvent.h"
#include <functional>

namespace vor {
    typedef std::list<VPoint*> Vertices;
    typedef std::list<VEdge*> Edges;

    class Voronoi {
    public:
        Voronoi();

        /*
            The only public function for generating a diagram

            input:
                    v        : Vertices - places for drawing a diagram
                    w        : width of the result (top left corner is (0, 0))
                    h        : height of the result
            output:
                    pointer to list of edges

            All the data structures are managed by this class
        */
        Edges* GetEdges(Vertices* v, int w, int h);

    private:
        /*
            places    : container of places with which we work
            edges    : container of edges which will be teh result
            width    : width of the diagram
            height    : height of the diagram
            root    : the root of the tree, that represents a beachline sequence
            ly        : current "y" position of the line (see Fortune's algorithm)
        */
        Edges* edges;
        VParabola* root;
        Vertices* places;
        double ly;
        double width;
        double height;

        /*
            deleted    : set  of deleted (false) Events (since we can not delete from PriorityQueue
            points    : list of all new points that were created during the algorithm
            queue    : priority queue with events to process
        */
        std::set<VEvent*> deleted;
        std::list<VPoint*> points;
        std::priority_queue<VEvent*, std::vector<VEvent*>, VEvent::CompareEvent> queue;

        /*
            InsertParabola      : processing the place event
            RemoveParabola      : processing the circle event
            FinishEdge        : recursively finishes all infinite edges in the tree
            GetXOfEdge        : returns the current x position of an intersection point of left and right parabolas
            GetParabolaByX      : returns the Parabola that is under this "x" position in the current beachline
            CheckCircle        : checks the circle event (disappearing) of this parabola
            GetEdgeIntersection
        */
        void InsertParabola(VPoint* p);
        void RemoveParabola(VEvent* e);
        VParabola* GetParabolaByX(double xx);

        void FinishEdge(VParabola* n);
        double GetXOfEdge(VParabola* par, double y);

        double GetY(VPoint* p, double x);
        void CheckCircle(VParabola* b);
        VPoint* GetEdgeIntersection(VEdge* a, VEdge* b);
    };
}

#endif
