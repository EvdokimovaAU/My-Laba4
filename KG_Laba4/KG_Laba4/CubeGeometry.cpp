#include "CubeGeometry.h"


static const Vertex g_vertices[] =
{
    // Front (z = -0.5)
    {{-0.5f,-0.5f,-0.5f},{0,0,-1}},
    {{-0.5f, 0.5f,-0.5f},{0,0,-1}},
    {{ 0.5f, 0.5f,-0.5f},{0,0,-1}},
    {{ 0.5f,-0.5f,-0.5f},{0,0,-1}},

    // Back (z = +0.5)
    {{-0.5f,-0.5f, 0.5f},{0,0, 1}},
    {{ 0.5f,-0.5f, 0.5f},{0,0, 1}},
    {{ 0.5f, 0.5f, 0.5f},{0,0, 1}},
    {{-0.5f, 0.5f, 0.5f},{0,0, 1}},

    // Left (x = -0.5)
    {{-0.5f,-0.5f, 0.5f},{-1,0,0}},
    {{-0.5f, 0.5f, 0.5f},{-1,0,0}},
    {{-0.5f, 0.5f,-0.5f},{-1,0,0}},
    {{-0.5f,-0.5f,-0.5f},{-1,0,0}},

    // Right (x = +0.5)
    {{ 0.5f,-0.5f,-0.5f},{ 1,0,0}},
    {{ 0.5f, 0.5f,-0.5f},{ 1,0,0}},
    {{ 0.5f, 0.5f, 0.5f},{ 1,0,0}},
    {{ 0.5f,-0.5f, 0.5f},{ 1,0,0}},

    // Top (y = +0.5)
    {{-0.5f, 0.5f,-0.5f},{0, 1,0}},
    {{-0.5f, 0.5f, 0.5f},{0, 1,0}},
    {{ 0.5f, 0.5f, 0.5f},{0, 1,0}},
    {{ 0.5f, 0.5f,-0.5f},{0, 1,0}},

    // Bottom (y = -0.5)
    {{-0.5f,-0.5f, 0.5f},{0,-1,0}},
    {{-0.5f,-0.5f,-0.5f},{0,-1,0}},
    {{ 0.5f,-0.5f,-0.5f},{0,-1,0}},
    {{ 0.5f,-0.5f, 0.5f},{0,-1,0}},
};

static const uint16_t g_indices[] =
{
     0,  1,  2,  0,  2,  3,
     4,  5,  6,  4,  6,  7,
     8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,
    16, 17, 18, 16, 18, 19,
    20, 21, 22, 20, 22, 23
};

const Vertex* CubeGeometry::Vertices() { return g_vertices; }
uint32_t CubeGeometry::VertexCount() { return (uint32_t)(sizeof(g_vertices) / sizeof(g_vertices[0])); }

const uint16_t* CubeGeometry::Indices() { return g_indices; }
uint32_t CubeGeometry::IndexCount() { return (uint32_t)(sizeof(g_indices) / sizeof(g_indices[0])); }
