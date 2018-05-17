#ifndef DIVIDE_H
#define DIVIDE_H

// I think it only computes lower hull, lel

struct dacVertex
{

    glm::vec3 vector;
    int vIndex;
    dacVertex *next;
    dacVertex *prev;
    void act()
    {
        //insert
        if (prev->next != this)
        {
            prev->next = next->prev = this;
        }
        //delete
        else
        {
            prev->next = next;
            next->prev = prev;
        }
    }
};

struct dacFace
{
    dacVertex *vertex[3];
    glm::vec3 normal;
    glm::vec3 centerPoint;
};

struct dac_context
{
    bool initialized;
    dacVertex *vertices;
    int numberOfPoints;
    std::vector<dacFace *> faces;
    mesh *m;
};

const double INF = 1e99;
static dacVertex nil = {glm::vec3(INF, INF, INF), 0, nullptr, nullptr};
dacVertex *NIL = &nil;

dacVertex *sort(dacVertex points[], int numberOfPoints)
{
    dacVertex *l, *r, *c, head;

    if (numberOfPoints == 1)
    {
        points[0].next = NIL;
        return points;
    }
    l = sort(points, numberOfPoints / 2);
    r = sort(points + (numberOfPoints / 2), numberOfPoints - (numberOfPoints / 2));
    c = &head;
    do
    {
        if (l->vector.x < r->vector.x)
        {
            c = c->next = l;
            l = l->next;
        }
        else
        {
            c = c->next = r;
            r = r->next;
        }
    } while (c != NIL);
    return head.next;
}

glm::vec3 dacComputeFaceNormal(dacFace *f)
{
    // Newell's Method
    // https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
    glm::vec3 normal = glm::vec3(0.0f);

    for (int i = 0; i < 3; i++)
    {
        glm::vec3 current = f->vertex[i]->vector;
        glm::vec3 next = f->vertex[(i + 1) % 3]->vector;

        normal.x = normal.x + (current.y - next.y) * (current.z + next.z);
        normal.y = normal.y + (current.z - next.z) * (current.x + next.x);
        normal.z = normal.z + (current.x - next.x) * (current.y + next.y);
    }

    return glm::normalize(normal);
}

dacFace *dacCreateFaceFromPoints(dacVertex *u, dacVertex *v, dacVertex *w)
{
    dacFace *f = (dacFace *)malloc(sizeof(dacFace));
    f->vertex[0] = u;
    f->vertex[1] = v;
    f->vertex[2] = w;
    f->normal = dacComputeFaceNormal(f);
    f->centerPoint = (u->vector + v->vector + w->vector) / 3.0f;

    return f;
}

static void dacCopyVertices(dac_context &dac, vertex *vertices, int numberOfPoints)
{
    dac.vertices = (dacVertex *)malloc(sizeof(dacVertex) * numberOfPoints);
    for (int i = 0; i < numberOfPoints; i++)
    {
        dac.vertices[i].vIndex = i;
        dac.vertices[i].vector = vertices[i].position;
    }
}

double orient(dacVertex *p, dacVertex *q, dacVertex *r)
{
    //orient(p,q,r)=det(1  p_x  p_y)
    //                 (1  q_x  q_y)
    //                 (1  r_x  r_y)
    //the determinant gives twice the signed area of the triangle formed by p, q and r
    //SO, this tests if three points do a ccw turn at time -INF

    if (p == NIL || q == NIL || r == NIL)
    {
        return 1.0;
    }
    return (q->vector.x - p->vector.x) * (r->vector.y - p->vector.y) - (r->vector.x - p->vector.x) * (q->vector.y - p->vector.y);
}

//by dividing with orient, we can determine the time when three points switch from cw to ccw (or the other way)
double time(dacVertex *p, dacVertex *q, dacVertex *r)
{
    if (p == NIL || q == NIL || r == NIL)
    {
        return INF;
    }

    return ((q->vector.x - p->vector.x) * (r->vector.z - p->vector.z) - (r->vector.x - p->vector.x) * (q->vector.z - p->vector.z)) / orient(p, q, r);
}

void dacHull(dacVertex *list, int n, dacVertex **A, dacVertex **B, bool lower)
{
    dacVertex *u, *v, *mid;
    int i, j, k, l, minl;

    if (n == 1)
    {
        A[0] = list->prev = list->next = NIL;
        return;
    }

    //find middle
    for (u = list, i = 0; i < n / 2 - 1; u = u->next, i++)
        ;
    mid = v = u->next;
    dacHull(list, n / 2, B, A, lower); // recurse on left and right sides
    dacHull(mid, n - n / 2, B + n / 2 * 2, A + n / 2 * 2, lower);

    // find initial bridge
    for (;;)
    {
        if (orient(u, v, v->next) < 0)
        {
            v = v->next;
            continue;
        }
        if (orient(u->prev, u, v) < 0)
        {
            u = u->prev;
            continue;
        }
        break;
    }

    i = k = 0;
    j = n / 2 * 2;

    double t[6], oldTime, newTime;
    oldTime = -INF;
    // merge by tracking bridge uv over time
    // infinite loop until no insertion/deletion events occur
    for (;;)
    {
        if (lower)
        {
            t[0] = time(B[i]->prev, B[i], B[i]->next);
            t[1] = time(B[j]->prev, B[j], B[j]->next);
            t[2] = time(u->prev, u, v);
            t[3] = time(u, u->next, v);
            t[4] = time(u, v, v->next);
            t[5] = time(u, v->prev, v);
        }
        else 
        {
            t[0] = -time(B[i]->prev, B[i], B[i]->next);
            t[1] = -time(B[j]->prev, B[j], B[j]->next);
            t[2] = -time(u->prev, u, v);
            t[3] = -time(u, u->next, v);
            t[4] = -time(u, v, v->next);
            t[5] = -time(u, v->prev, v);
        }

        //we find the movies in chronological time
        newTime = INF;
        for (l = 0; l < 6; l++)
        {
            if (t[l] > oldTime && t[l] < newTime)
            {
                minl = l;
                newTime = t[l];
            }
        }
        //if newTime==INF, no insertion/deletion events occured, and we break for loop
        if (newTime == INF)
        {
            break;
        }
        //act on the smallest time value
        switch (minl)
        {
        case 0:
            //insert or delete of w in L. If w is to the left of u, insert or delete w in A.
            if (B[i]->vector.x < u->vector.x)
            {
                A[k] = B[i];
                k++;
            }
            B[i]->act();
            i++;
            break;
        case 1:
            //insert or delete of w in R. If w is to the right of v, insert or delete w in A.
            if (B[j]->vector.x > v->vector.x)
            {
                A[k] = B[j];
                k++;
            }
            B[j]->act();
            j++;
            break;
        case 2:
            //u->prev, u, v was ccw and has turned cw, so u->prev and v is the new bridge, and we delete u in A.
            A[k] = u;
            k++;
            u = u->prev;
            break;
        case 3:
            //u, u->next, v, was cw and turned ccw, so u->next and v is the new bridge, and we insert u->next between u and v.
            A[k] = u = u->next;
            k++;
            break;
        case 4:
            //u, v, v->next was ccw and turned cw, so u and v->next is the new bridge, and we delete v in A.
            A[k] = v;
            v = v->next;
            k++;
            break;
        case 5:
            //u, v->prev, v, was cw and turned ccw, so u and v->prev is the new bridge, and we insert v->prev between u and v.
            A[k] = v = v->prev;
            k++;
            break;
        }
        oldTime = newTime;
    }
    A[k] = NIL;

    //connect the bridge uv
    u->next = v;
    v->prev = u;

    // now go back in time to update pointers
    // during insertion of q between p and r, we cannot store p and r in the prev and next fields, as they are still in use in L and R
    for (k--; k >= 0; k--)
    {
        if (A[k]->vector.x <= u->vector.x || A[k]->vector.x >= v->vector.x)
        {
            A[k]->act();
            if (A[k] == u)
            {
                u = u->prev;
            }
            else if (A[k] == v)
            {
                v = v->next;
            }
        }
        else
        {
            u->next = A[k];
            A[k]->prev = u;
            v->prev = A[k];
            A[k]->next = v;
            if (A[k]->vector.x < mid->vector.x)
            {
                u = A[k];
            }
            else
            {
                v = A[k];
            }
        }
    }
}

mesh &dacConvertToMesh(dac_context &context, render_context &renderContext)
{
    /*  This version of the code prints the facets of the 3-d lower hull. This is done by
        processing the events in the output array A and tracking the linked list for the 2-d hull using the
        same prev and next fields.  */

    if (!context.m)
    {
        context.m = &InitEmptyMesh(renderContext);
    }

    for (const auto &f : context.faces)
    {
        face newFace = {};
        for (int i = 0; i < 3; i++)
        {
            vertex newVertex = {};
            newVertex.position = f->vertex[i]->vector;
            newVertex.vertexIndex = f->vertex[i]->vIndex;
            newFace.vertices[i] = newVertex;
        }
        newFace.faceColor = glm::vec4(0.0f, 1.0f, 1.0f, 0.7f);
        newFace.faceColor.w = 0.5f;
        newFace.faceNormal = f->normal;
        newFace.centerPoint = f->centerPoint;
        context.m->faces.push_back(newFace);
    }
    context.m->position = glm::vec3(0.0f);
    context.m->scale = glm::vec3(globalScale);
    context.m->dirty = true;

    return *context.m;
}

void dacConstructFullHull(dac_context &dacContext)
{
    //do the stuff for divide and conquer
    int i;
    int n = dacContext.numberOfPoints;

    dacVertex *P = dacContext.vertices;
    dacVertex *upperP = (dacVertex *)malloc(sizeof(dacVertex) * n);
    memcpy(upperP, P, sizeof(dacVertex) * n);
    dacVertex *list = sort(P, n);

    //Each vertex is inserted at most once and deleted at most once, so at most 2n events (facets).
    dacVertex **A = new dacVertex *[2 * n];
    //work array
    dacVertex **B = new dacVertex *[2 * n];
    dacHull(list, n, A, B, true);

    //create faces by processing the events in event array A
    for (i = 0; A[i] != NIL; A[i++]->act())
    {
        dacFace *newFace = dacCreateFaceFromPoints(A[i]->prev, A[i], A[i]->next);
        dacContext.faces.push_back(newFace);
    }

    dacVertex *upperList = sort(upperP, n);
    dacVertex **C = new dacVertex *[2 * n];
    //work array
    dacVertex **D = new dacVertex *[2 * n];
    dacHull(upperList, n, C, D, false);

    for (i = 0; C[i] != NIL; C[i++]->act())
    {
        dacFace *newFace = dacCreateFaceFromPoints(C[i]->prev, C[i], C[i]->next);
        dacContext.faces.push_back(newFace);
    }

    /*
    delete A;
    delete B;
    delete C;
    delete D;
    */
}

void dacHullStep(dac_context &dacContext)
{
    //do the stuff for one divide and conquer iteration
}

void dacInitializeContext(dac_context &dacContext, vertex *vertices, int numberOfPoints)
{
    //initialize this shit
    dacContext.numberOfPoints = numberOfPoints;
    dacContext.initialized = true;
    dacCopyVertices(dacContext, vertices, numberOfPoints);
}

#endif