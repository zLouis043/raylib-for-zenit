/**********************************************************************************************
*
*   raylib.models - Basic functions to deal with 3d shapes and 3d models
*
*   CONFIGURATION:
*
*   #define SUPPORT_FILEFORMAT_OBJ
*   #define SUPPORT_FILEFORMAT_MTL
*   #define SUPPORT_FILEFORMAT_IQM
*   #define SUPPORT_FILEFORMAT_GLTF
*       Selected desired fileformats to be supported for model data loading.
*
*   #define SUPPORT_MESH_GENERATION
*       Support procedural mesh generation functions, uses external par_shapes.h library
*       NOTE: Some generated meshes DO NOT include generated texture coordinates
*
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2014-2018 Ramon Santamaria (@raysan5)
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "raylib.h"         // Declares module functions

// Check if config flags have been externally provided on compilation line
#if !defined(EXTERNAL_CONFIG_FLAGS)
    #include "config.h"         // Defines module configuration flags
#endif

#include "utils.h"          // Required for: fopen() Android mapping

#include <stdio.h>          // Required for: FILE, fopen(), fclose(), fscanf(), feof(), rewind(), fgets()
#include <stdlib.h>         // Required for: malloc(), free()
#include <string.h>         // Required for: strcmp()
#include <math.h>           // Required for: sin(), cos()

#include "rlgl.h"           // raylib OpenGL abstraction layer to OpenGL 1.1, 2.1, 3.3+ or ES2

#if defined(SUPPORT_FILEFORMAT_OBJ) || defined(SUPPORT_FILEFORMAT_MTL)
    #define TINYOBJ_LOADER_C_IMPLEMENTATION
    #include "external/tinyobj_loader_c.h"      // OBJ/MTL file formats loading
#endif

#if defined(SUPPORT_FILEFORMAT_GLTF)
    #define CGLTF_IMPLEMENTATION
    #include "external/cgltf.h"         // glTF file format loading
#endif

#if defined(SUPPORT_MESH_GENERATION)
    #define PAR_SHAPES_IMPLEMENTATION
    #include "external/par_shapes.h"    // Shapes 3d parametric generation
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
#if defined(SUPPORT_FILEFORMAT_OBJ)
static Model LoadOBJ(const char *fileName);     // Load OBJ mesh data
#endif
#if defined(SUPPORT_FILEFORMAT_GLTF)
static Model LoadIQM(const char *fileName);     // Load IQM mesh data
#endif
#if defined(SUPPORT_FILEFORMAT_GLTF)
static Model LoadGLTF(const char *fileName);    // Load GLTF mesh data
#endif

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------

// Draw a line in 3D world space
void DrawLine3D(Vector3 startPos, Vector3 endPos, Color color)
{
    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(startPos.x, startPos.y, startPos.z);
        rlVertex3f(endPos.x, endPos.y, endPos.z);
    rlEnd();
}

// Draw a circle in 3D world space
void DrawCircle3D(Vector3 center, float radius, Vector3 rotationAxis, float rotationAngle, Color color)
{
    if (rlCheckBufferLimit(2*36)) rlglDraw();

    rlPushMatrix();
        rlTranslatef(center.x, center.y, center.z);
        rlRotatef(rotationAngle, rotationAxis.x, rotationAxis.y, rotationAxis.z);

        rlBegin(RL_LINES);
            for (int i = 0; i < 360; i += 10)
            {
                rlColor4ub(color.r, color.g, color.b, color.a);

                rlVertex3f(sinf(DEG2RAD*i)*radius, cosf(DEG2RAD*i)*radius, 0.0f);
                rlVertex3f(sinf(DEG2RAD*(i + 10))*radius, cosf(DEG2RAD*(i + 10))*radius, 0.0f);
            }
        rlEnd();
    rlPopMatrix();
}

// Draw cube
// NOTE: Cube position is the center position
void DrawCube(Vector3 position, float width, float height, float length, Color color)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (rlCheckBufferLimit(36)) rlglDraw();

    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
        rlTranslatef(position.x, position.y, position.z);
        //rlRotatef(45, 0, 1, 0);
        //rlScalef(1.0f, 1.0f, 1.0f);   // NOTE: Vertices are directly scaled on definition

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            // Front face
            rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left

            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Right
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right

            // Back face
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Left
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right

            rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left

            // Top face
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Bottom Left
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right

            rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right

            // Bottom face
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Left
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
            rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left

            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Top Right
            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Left

            // Right face
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left

            rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left
            rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left

            // Left face
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left
            rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Right

            rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left
            rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left
            rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right
        rlEnd();
    rlPopMatrix();
}

// Draw cube (Vector version)
void DrawCubeV(Vector3 position, Vector3 size, Color color)
{
    DrawCube(position, size.x, size.y, size.z, color);
}

// Draw cube wires
void DrawCubeWires(Vector3 position, float width, float height, float length, Color color)
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    if (rlCheckBufferLimit(36)) rlglDraw();

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            // Front Face -----------------------------------------------------
            // Bottom Line
            rlVertex3f(x-width/2, y-height/2, z+length/2);  // Bottom Left
            rlVertex3f(x+width/2, y-height/2, z+length/2);  // Bottom Right

            // Left Line
            rlVertex3f(x+width/2, y-height/2, z+length/2);  // Bottom Right
            rlVertex3f(x+width/2, y+height/2, z+length/2);  // Top Right

            // Top Line
            rlVertex3f(x+width/2, y+height/2, z+length/2);  // Top Right
            rlVertex3f(x-width/2, y+height/2, z+length/2);  // Top Left

            // Right Line
            rlVertex3f(x-width/2, y+height/2, z+length/2);  // Top Left
            rlVertex3f(x-width/2, y-height/2, z+length/2);  // Bottom Left

            // Back Face ------------------------------------------------------
            // Bottom Line
            rlVertex3f(x-width/2, y-height/2, z-length/2);  // Bottom Left
            rlVertex3f(x+width/2, y-height/2, z-length/2);  // Bottom Right

            // Left Line
            rlVertex3f(x+width/2, y-height/2, z-length/2);  // Bottom Right
            rlVertex3f(x+width/2, y+height/2, z-length/2);  // Top Right

            // Top Line
            rlVertex3f(x+width/2, y+height/2, z-length/2);  // Top Right
            rlVertex3f(x-width/2, y+height/2, z-length/2);  // Top Left

            // Right Line
            rlVertex3f(x-width/2, y+height/2, z-length/2);  // Top Left
            rlVertex3f(x-width/2, y-height/2, z-length/2);  // Bottom Left

            // Top Face -------------------------------------------------------
            // Left Line
            rlVertex3f(x-width/2, y+height/2, z+length/2);  // Top Left Front
            rlVertex3f(x-width/2, y+height/2, z-length/2);  // Top Left Back

            // Right Line
            rlVertex3f(x+width/2, y+height/2, z+length/2);  // Top Right Front
            rlVertex3f(x+width/2, y+height/2, z-length/2);  // Top Right Back

            // Bottom Face  ---------------------------------------------------
            // Left Line
            rlVertex3f(x-width/2, y-height/2, z+length/2);  // Top Left Front
            rlVertex3f(x-width/2, y-height/2, z-length/2);  // Top Left Back

            // Right Line
            rlVertex3f(x+width/2, y-height/2, z+length/2);  // Top Right Front
            rlVertex3f(x+width/2, y-height/2, z-length/2);  // Top Right Back
        rlEnd();
    rlPopMatrix();
}

// Draw cube wires (vector version)
void DrawCubeWiresV(Vector3 position, Vector3 size, Color color)
{
    DrawCubeWires(position, size.x, size.y, size.z, color);
}

// Draw cube
// NOTE: Cube position is the center position
void DrawCubeTexture(Texture2D texture, Vector3 position, float width, float height, float length, Color color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    rlEnableTexture(texture.id);

    //rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> rotate -> translate)
        //rlTranslatef(2.0f, 0.0f, 0.0f);
        //rlRotatef(45, 0, 1, 0);
        //rlScalef(2.0f, 2.0f, 2.0f);

        rlBegin(RL_QUADS);
            rlColor4ub(color.r, color.g, color.b, color.a);
            // Front Face
            rlNormal3f(0.0f, 0.0f, 1.0f);                  // Normal Pointing Towards Viewer
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Left Of The Texture and Quad
            // Back Face
            rlNormal3f(0.0f, 0.0f, - 1.0f);                  // Normal Pointing Away From Viewer
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Left Of The Texture and Quad
            // Top Face
            rlNormal3f(0.0f, 1.0f, 0.0f);                  // Normal Pointing Up
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right Of The Texture and Quad
            // Bottom Face
            rlNormal3f(0.0f, - 1.0f, 0.0f);                  // Normal Pointing Down
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            // Right face
            rlNormal3f(1.0f, 0.0f, 0.0f);                  // Normal Pointing Right
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z - length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z - length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x + width/2, y + height/2, z + length/2);  // Top Left Of The Texture and Quad
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x + width/2, y - height/2, z + length/2);  // Bottom Left Of The Texture and Quad
            // Left Face
            rlNormal3f( - 1.0f, 0.0f, 0.0f);                  // Normal Pointing Left
            rlTexCoord2f(0.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z - length/2);  // Bottom Left Of The Texture and Quad
            rlTexCoord2f(1.0f, 0.0f); rlVertex3f(x - width/2, y - height/2, z + length/2);  // Bottom Right Of The Texture and Quad
            rlTexCoord2f(1.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z + length/2);  // Top Right Of The Texture and Quad
            rlTexCoord2f(0.0f, 1.0f); rlVertex3f(x - width/2, y + height/2, z - length/2);  // Top Left Of The Texture and Quad
        rlEnd();
    //rlPopMatrix();

    rlDisableTexture();
}

// Draw sphere
void DrawSphere(Vector3 centerPos, float radius, Color color)
{
    DrawSphereEx(centerPos, radius, 16, 16, color);
}

// Draw sphere with extended parameters
void DrawSphereEx(Vector3 centerPos, float radius, int rings, int slices, Color color)
{
    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> translate)
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(radius, radius, radius);

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < (rings + 2); i++)
            {
                for (int j = 0; j < slices; j++)
                {
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*i))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*i)),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*i))*cosf(DEG2RAD*(j*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*((j+1)*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*((j+1)*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*(j*360/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*i))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*i)),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*i))*cosf(DEG2RAD*(j*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i)))*sinf(DEG2RAD*((j+1)*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i)))*cosf(DEG2RAD*((j+1)*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*((j+1)*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*((j+1)*360/slices)));
                }
            }
        rlEnd();
    rlPopMatrix();
}

// Draw sphere wires
void DrawSphereWires(Vector3 centerPos, float radius, int rings, int slices, Color color)
{
    rlPushMatrix();
        // NOTE: Transformation is applied in inverse order (scale -> translate)
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(radius, radius, radius);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < (rings + 2); i++)
            {
                for (int j = 0; j < slices; j++)
                {
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*i))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*i)),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*i))*cosf(DEG2RAD*(j*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*((j+1)*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*((j+1)*360/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*((j+1)*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*((j+1)*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*(j*360/slices)));

                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*(i+1))),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*(i+1)))*cosf(DEG2RAD*(j*360/slices)));
                    rlVertex3f(cosf(DEG2RAD*(270+(180/(rings + 1))*i))*sinf(DEG2RAD*(j*360/slices)),
                               sinf(DEG2RAD*(270+(180/(rings + 1))*i)),
                               cosf(DEG2RAD*(270+(180/(rings + 1))*i))*cosf(DEG2RAD*(j*360/slices)));
                }
            }
        rlEnd();
    rlPopMatrix();
}

// Draw a cylinder
// NOTE: It could be also used for pyramid and cone
void DrawCylinder(Vector3 position, float radiusTop, float radiusBottom, float height, int sides, Color color)
{
    if (sides < 3) sides = 3;

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_TRIANGLES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            if (radiusTop > 0)
            {
                // Draw Body -------------------------------------------------------------------------------------
                for (int i = 0; i < 360; i += 360/sides)
                {
                    rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom); //Bottom Left
                    rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360/sides))*radiusBottom); //Bottom Right
                    rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360/sides))*radiusTop); //Top Right

                    rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop); //Top Left
                    rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom); //Bottom Left
                    rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360/sides))*radiusTop); //Top Right
                }

                // Draw Cap --------------------------------------------------------------------------------------
                for (int i = 0; i < 360; i += 360/sides)
                {
                    rlVertex3f(0, height, 0);
                    rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop);
                    rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360/sides))*radiusTop);
                }
            }
            else
            {
                // Draw Cone -------------------------------------------------------------------------------------
                for (int i = 0; i < 360; i += 360/sides)
                {
                    rlVertex3f(0, height, 0);
                    rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
                    rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360/sides))*radiusBottom);
                }
            }

            // Draw Base -----------------------------------------------------------------------------------------
            for (int i = 0; i < 360; i += 360/sides)
            {
                rlVertex3f(0, 0, 0);
                rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360/sides))*radiusBottom);
                rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
            }
        rlEnd();
    rlPopMatrix();
}

// Draw a wired cylinder
// NOTE: It could be also used for pyramid and cone
void DrawCylinderWires(Vector3 position, float radiusTop, float radiusBottom, float height, int sides, Color color)
{
    if (sides < 3) sides = 3;

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);

        rlBegin(RL_LINES);
            rlColor4ub(color.r, color.g, color.b, color.a);

            for (int i = 0; i < 360; i += 360/sides)
            {
                rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
                rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360/sides))*radiusBottom);

                rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusBottom, 0, cosf(DEG2RAD*(i + 360/sides))*radiusBottom);
                rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360/sides))*radiusTop);

                rlVertex3f(sinf(DEG2RAD*(i + 360/sides))*radiusTop, height, cosf(DEG2RAD*(i + 360/sides))*radiusTop);
                rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop);

                rlVertex3f(sinf(DEG2RAD*i)*radiusTop, height, cosf(DEG2RAD*i)*radiusTop);
                rlVertex3f(sinf(DEG2RAD*i)*radiusBottom, 0, cosf(DEG2RAD*i)*radiusBottom);
            }
        rlEnd();
    rlPopMatrix();
}

// Draw a plane
void DrawPlane(Vector3 centerPos, Vector2 size, Color color)
{
    // NOTE: Plane is always created on XZ ground
    rlPushMatrix();
        rlTranslatef(centerPos.x, centerPos.y, centerPos.z);
        rlScalef(size.x, 1.0f, size.y);

        rlBegin(RL_QUADS);
            rlColor4ub(color.r, color.g, color.b, color.a);
            rlNormal3f(0.0f, 1.0f, 0.0f);

            rlVertex3f(-0.5f, 0.0f, -0.5f);
            rlVertex3f(-0.5f, 0.0f, 0.5f);
            rlVertex3f(0.5f, 0.0f, 0.5f);
            rlVertex3f(0.5f, 0.0f, -0.5f);
        rlEnd();
    rlPopMatrix();
}

// Draw a ray line
void DrawRay(Ray ray, Color color)
{
    float scale = 10000;

    rlBegin(RL_LINES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlColor4ub(color.r, color.g, color.b, color.a);

        rlVertex3f(ray.position.x, ray.position.y, ray.position.z);
        rlVertex3f(ray.position.x + ray.direction.x*scale, ray.position.y + ray.direction.y*scale, ray.position.z + ray.direction.z*scale);
    rlEnd();
}

// Draw a grid centered at (0, 0, 0)
void DrawGrid(int slices, float spacing)
{
    int halfSlices = slices/2;

    rlBegin(RL_LINES);
        for (int i = -halfSlices; i <= halfSlices; i++)
        {
            if (i == 0)
            {
                rlColor3f(0.5f, 0.5f, 0.5f);
                rlColor3f(0.5f, 0.5f, 0.5f);
                rlColor3f(0.5f, 0.5f, 0.5f);
                rlColor3f(0.5f, 0.5f, 0.5f);
            }
            else
            {
                rlColor3f(0.75f, 0.75f, 0.75f);
                rlColor3f(0.75f, 0.75f, 0.75f);
                rlColor3f(0.75f, 0.75f, 0.75f);
                rlColor3f(0.75f, 0.75f, 0.75f);
            }

            rlVertex3f((float)i*spacing, 0.0f, (float)-halfSlices*spacing);
            rlVertex3f((float)i*spacing, 0.0f, (float)halfSlices*spacing);

            rlVertex3f((float)-halfSlices*spacing, 0.0f, (float)i*spacing);
            rlVertex3f((float)halfSlices*spacing, 0.0f, (float)i*spacing);
        }
    rlEnd();
}

// Draw gizmo
void DrawGizmo(Vector3 position)
{
    // NOTE: RGB = XYZ
    float length = 1.0f;

    rlPushMatrix();
        rlTranslatef(position.x, position.y, position.z);
        rlScalef(length, length, length);

        rlBegin(RL_LINES);
            rlColor3f(1.0f, 0.0f, 0.0f); rlVertex3f(0.0f, 0.0f, 0.0f);
            rlColor3f(1.0f, 0.0f, 0.0f); rlVertex3f(1.0f, 0.0f, 0.0f);

            rlColor3f(0.0f, 1.0f, 0.0f); rlVertex3f(0.0f, 0.0f, 0.0f);
            rlColor3f(0.0f, 1.0f, 0.0f); rlVertex3f(0.0f, 1.0f, 0.0f);

            rlColor3f(0.0f, 0.0f, 1.0f); rlVertex3f(0.0f, 0.0f, 0.0f);
            rlColor3f(0.0f, 0.0f, 1.0f); rlVertex3f(0.0f, 0.0f, 1.0f);
        rlEnd();
    rlPopMatrix();
}

// Load model from files (mesh and material)
Model LoadModel(const char *fileName)
{
    Model model = { 0 };

#if defined(SUPPORT_FILEFORMAT_OBJ)
    if (IsFileExtension(fileName, ".obj")) model = LoadOBJ(fileName);
#endif
#if defined(SUPPORT_FILEFORMAT_GLTF)
    if (IsFileExtension(fileName, ".gltf")) model = LoadGLTF(fileName);
#endif
#if defined(SUPPORT_FILEFORMAT_IQM)
    if (IsFileExtension(fileName, ".iqm")) model = LoadIQM(fileName);
#endif

    // Make sure model transform is set to identity matrix!
    model.transform = MatrixIdentity();

    if (model.meshCount == 0)
    {
        TraceLog(LOG_WARNING, "[%s] No meshes can be loaded, default to cube mesh", fileName);

        model.meshCount = 1;
        model.meshes = (Mesh *)calloc(model.meshCount, sizeof(Mesh));
        model.meshes[0] = GenMeshCube(1.0f, 1.0f, 1.0f);
    }
    else
    {
        // Upload vertex data to GPU (static mesh)
        for (int i = 0; i < model.meshCount; i++) rlLoadMesh(&model.meshes[i], false);
    }

    if (model.materialCount == 0)
    {
        TraceLog(LOG_WARNING, "[%s] No materials can be loaded, default to white material", fileName);

        model.materialCount = 1;
        model.materials = (Material *)calloc(model.materialCount, sizeof(Material));
        model.materials[0] = LoadMaterialDefault();

        model.meshMaterial = (int *)calloc(model.meshCount, sizeof(int));
    }

    return model;
}

// Load model from generated mesh
// WARNING: A shallow copy of mesh is generated, passed by value,
// as long as struct contains pointers to data and some values, we get a copy
// of mesh pointing to same data as original version... be careful!
Model LoadModelFromMesh(Mesh mesh)
{
    Model model = { 0 };

    model.transform = MatrixIdentity();

    model.meshCount = 1;
    model.meshes = (Mesh *)malloc(model.meshCount*sizeof(Mesh));
    model.meshes[0] = mesh;

    model.materialCount = 1;
    model.materials = (Material *)malloc(model.materialCount*sizeof(Material));
    model.materials[0] = LoadMaterialDefault();

    model.meshMaterial = (int *)malloc(model.meshCount*sizeof(int));
    model.meshMaterial[0] = 0;  // First material index

    return model;
}

// Unload model from memory (RAM and/or VRAM)
void UnloadModel(Model model)
{
    for (int i = 0; i < model.meshCount; i++) UnloadMesh(&model.meshes[i]);
    for (int i = 0; i < model.materialCount; i++) UnloadMaterial(model.materials[i]);

    free(model.meshes);
    free(model.materials);
    free(model.meshMaterial);

    // Unload animation data
    free(model.bones);
    free(model.bindPose);

    TraceLog(LOG_INFO, "Unloaded model data from RAM and VRAM");
}

// Unload mesh from memory (RAM and/or VRAM)
void UnloadMesh(Mesh *mesh)
{
    rlUnloadMesh(mesh);
}

// Export mesh data to file
void ExportMesh(Mesh mesh, const char *fileName)
{
    bool success = false;

    if (IsFileExtension(fileName, ".obj"))
    {
        FILE *objFile = fopen(fileName, "wt");

        fprintf(objFile, "# //////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(objFile, "# //                                                                              //\n");
        fprintf(objFile, "# // rMeshOBJ exporter v1.0 - Mesh exported as triangle faces and not optimized   //\n");
        fprintf(objFile, "# //                                                                              //\n");
        fprintf(objFile, "# // more info and bugs-report:  github.com/raysan5/raylib                        //\n");
        fprintf(objFile, "# // feedback and support:       ray[at]raylib.com                                //\n");
        fprintf(objFile, "# //                                                                              //\n");
        fprintf(objFile, "# // Copyright (c) 2018 Ramon Santamaria (@raysan5)                               //\n");
        fprintf(objFile, "# //                                                                              //\n");
        fprintf(objFile, "# //////////////////////////////////////////////////////////////////////////////////\n\n");
        fprintf(objFile, "# Vertex Count:     %i\n", mesh.vertexCount);
        fprintf(objFile, "# Triangle Count:   %i\n\n", mesh.triangleCount);

        fprintf(objFile, "g mesh\n");

        for (int i = 0, v = 0; i < mesh.vertexCount; i++, v += 3)
        {
            fprintf(objFile, "v %.2f %.2f %.2f\n", mesh.vertices[v], mesh.vertices[v + 1], mesh.vertices[v + 2]);
        }

        for (int i = 0, v = 0; i < mesh.vertexCount; i++, v += 2)
        {
            fprintf(objFile, "vt %.2f %.2f\n", mesh.texcoords[v], mesh.texcoords[v + 1]);
        }

        for (int i = 0, v = 0; i < mesh.vertexCount; i++, v += 3)
        {
            fprintf(objFile, "vn %.2f %.2f %.2f\n", mesh.normals[v], mesh.normals[v + 1], mesh.normals[v + 2]);
        }

        for (int i = 0; i < mesh.triangleCount; i += 3)
        {
            fprintf(objFile, "f %i/%i/%i %i/%i/%i %i/%i/%i\n", i, i, i, i + 1, i + 1, i + 1, i + 2, i + 2, i + 2);
        }

        fprintf(objFile, "\n");

        fclose(objFile);

        success = true;
    }
    else if (IsFileExtension(fileName, ".raw")) { }   // TODO: Support additional file formats to export mesh vertex data

    if (success) TraceLog(LOG_INFO, "Mesh exported successfully: %s", fileName);
    else TraceLog(LOG_WARNING, "Mesh could not be exported.");
}

#if defined(SUPPORT_MESH_GENERATION)
// Generate polygonal mesh
Mesh GenMeshPoly(int sides, float radius)
{
    Mesh mesh = { 0 };
    int vertexCount = sides*3;

    // Vertices definition
    Vector3 *vertices = (Vector3 *)malloc(vertexCount*sizeof(Vector3));
    for (int i = 0, v = 0; i < 360; i += 360/sides, v += 3)
    {
        vertices[v] = (Vector3){ 0.0f, 0.0f, 0.0f };
        vertices[v + 1] = (Vector3){ sinf(DEG2RAD*i)*radius, 0.0f, cosf(DEG2RAD*i)*radius };
        vertices[v + 2] = (Vector3){ sinf(DEG2RAD*(i + 360/sides))*radius, 0.0f, cosf(DEG2RAD*(i + 360/sides))*radius };
    }

    // Normals definition
    Vector3 *normals = (Vector3 *)malloc(vertexCount*sizeof(Vector3));
    for (int n = 0; n < vertexCount; n++) normals[n] = (Vector3){ 0.0f, 1.0f, 0.0f };   // Vector3.up;

    // TexCoords definition
    Vector2 *texcoords = (Vector2 *)malloc(vertexCount*sizeof(Vector2));
    for (int n = 0; n < vertexCount; n++) texcoords[n] = (Vector2){ 0.0f, 0.0f };

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = sides;
    mesh.vertices = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)malloc(mesh.vertexCount*2*sizeof(float));
    mesh.normals = (float *)malloc(mesh.vertexCount*3*sizeof(float));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.vertices[3*i] = vertices[i].x;
        mesh.vertices[3*i + 1] = vertices[i].y;
        mesh.vertices[3*i + 2] = vertices[i].z;
    }

    // Mesh texcoords array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.texcoords[2*i] = texcoords[i].x;
        mesh.texcoords[2*i + 1] = texcoords[i].y;
    }

    // Mesh normals array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.normals[3*i] = normals[i].x;
        mesh.normals[3*i + 1] = normals[i].y;
        mesh.normals[3*i + 2] = normals[i].z;
    }

    free(vertices);
    free(normals);
    free(texcoords);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate plane mesh (with subdivisions)
Mesh GenMeshPlane(float width, float length, int resX, int resZ)
{
    Mesh mesh = { 0 };

#define CUSTOM_MESH_GEN_PLANE
#if defined(CUSTOM_MESH_GEN_PLANE)
    resX++;
    resZ++;

    // Vertices definition
    int vertexCount = resX*resZ; // vertices get reused for the faces

    Vector3 *vertices = (Vector3 *)malloc(vertexCount*sizeof(Vector3));
    for (int z = 0; z < resZ; z++)
    {
        // [-length/2, length/2]
        float zPos = ((float)z/(resZ - 1) - 0.5f)*length;
        for (int x = 0; x < resX; x++)
        {
            // [-width/2, width/2]
            float xPos = ((float)x/(resX - 1) - 0.5f)*width;
            vertices[x + z*resX] = (Vector3){ xPos, 0.0f, zPos };
        }
    }

    // Normals definition
    Vector3 *normals = (Vector3 *)malloc(vertexCount*sizeof(Vector3));
    for (int n = 0; n < vertexCount; n++) normals[n] = (Vector3){ 0.0f, 1.0f, 0.0f };   // Vector3.up;

    // TexCoords definition
    Vector2 *texcoords = (Vector2 *)malloc(vertexCount*sizeof(Vector2));
    for (int v = 0; v < resZ; v++)
    {
        for (int u = 0; u < resX; u++)
        {
            texcoords[u + v*resX] = (Vector2){ (float)u/(resX - 1), (float)v/(resZ - 1) };
        }
    }

    // Triangles definition (indices)
    int numFaces = (resX - 1)*(resZ - 1);
    int *triangles = (int *)malloc(numFaces*6*sizeof(int));
    int t = 0;
    for (int face = 0; face < numFaces; face++)
    {
        // Retrieve lower left corner from face ind
        int i = face % (resX - 1) + (face/(resZ - 1)*resX);

        triangles[t++] = i + resX;
        triangles[t++] = i + 1;
        triangles[t++] = i;

        triangles[t++] = i + resX;
        triangles[t++] = i + resX + 1;
        triangles[t++] = i + 1;
    }

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = numFaces*2;
    mesh.vertices = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)malloc(mesh.vertexCount*2*sizeof(float));
    mesh.normals = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.indices = (unsigned short *)malloc(mesh.triangleCount*3*sizeof(unsigned short));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.vertices[3*i] = vertices[i].x;
        mesh.vertices[3*i + 1] = vertices[i].y;
        mesh.vertices[3*i + 2] = vertices[i].z;
    }

    // Mesh texcoords array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.texcoords[2*i] = texcoords[i].x;
        mesh.texcoords[2*i + 1] = texcoords[i].y;
    }

    // Mesh normals array
    for (int i = 0; i < mesh.vertexCount; i++)
    {
        mesh.normals[3*i] = normals[i].x;
        mesh.normals[3*i + 1] = normals[i].y;
        mesh.normals[3*i + 2] = normals[i].z;
    }

    // Mesh indices array initialization
    for (int i = 0; i < mesh.triangleCount*3; i++) mesh.indices[i] = triangles[i];

    free(vertices);
    free(normals);
    free(texcoords);
    free(triangles);

#else       // Use par_shapes library to generate plane mesh

    par_shapes_mesh *plane = par_shapes_create_plane(resX, resZ);   // No normals/texcoords generated!!!
    par_shapes_scale(plane, width, length, 1.0f);
    par_shapes_rotate(plane, -PI/2.0f, (float[]){ 1, 0, 0 });
    par_shapes_translate(plane, -width/2, 0.0f, length/2);

    mesh.vertices = (float *)malloc(plane->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(plane->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(plane->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = plane->ntriangles*3;
    mesh.triangleCount = plane->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = plane->points[plane->triangles[k]*3];
        mesh.vertices[k*3 + 1] = plane->points[plane->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = plane->points[plane->triangles[k]*3 + 2];

        mesh.normals[k*3] = plane->normals[plane->triangles[k]*3];
        mesh.normals[k*3 + 1] = plane->normals[plane->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = plane->normals[plane->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = plane->tcoords[plane->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = plane->tcoords[plane->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(plane);
#endif

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generated cuboid mesh
Mesh GenMeshCube(float width, float height, float length)
{
    Mesh mesh = { 0 };

#define CUSTOM_MESH_GEN_CUBE
#if defined(CUSTOM_MESH_GEN_CUBE)
    float vertices[] = {
        -width/2, -height/2, length/2,
        width/2, -height/2, length/2,
        width/2, height/2, length/2,
        -width/2, height/2, length/2,
        -width/2, -height/2, -length/2,
        -width/2, height/2, -length/2,
        width/2, height/2, -length/2,
        width/2, -height/2, -length/2,
        -width/2, height/2, -length/2,
        -width/2, height/2, length/2,
        width/2, height/2, length/2,
        width/2, height/2, -length/2,
        -width/2, -height/2, -length/2,
        width/2, -height/2, -length/2,
        width/2, -height/2, length/2,
        -width/2, -height/2, length/2,
        width/2, -height/2, -length/2,
        width/2, height/2, -length/2,
        width/2, height/2, length/2,
        width/2, -height/2, length/2,
        -width/2, -height/2, -length/2,
        -width/2, -height/2, length/2,
        -width/2, height/2, length/2,
        -width/2, height/2, -length/2
    };

    float texcoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f
    };

    mesh.vertices = (float *)malloc(24*3*sizeof(float));
    memcpy(mesh.vertices, vertices, 24*3*sizeof(float));

    mesh.texcoords = (float *)malloc(24*2*sizeof(float));
    memcpy(mesh.texcoords, texcoords, 24*2*sizeof(float));

    mesh.normals = (float *)malloc(24*3*sizeof(float));
    memcpy(mesh.normals, normals, 24*3*sizeof(float));

    mesh.indices = (unsigned short *)malloc(36*sizeof(unsigned short));

    int k = 0;

    // Indices can be initialized right now
    for (int i = 0; i < 36; i+=6)
    {
        mesh.indices[i] = 4*k;
        mesh.indices[i+1] = 4*k+1;
        mesh.indices[i+2] = 4*k+2;
        mesh.indices[i+3] = 4*k;
        mesh.indices[i+4] = 4*k+2;
        mesh.indices[i+5] = 4*k+3;

        k++;
    }

    mesh.vertexCount = 24;
    mesh.triangleCount = 12;

#else               // Use par_shapes library to generate cube mesh
/*
// Platonic solids:
par_shapes_mesh* par_shapes_create_tetrahedron();       // 4 sides polyhedron (pyramid)
par_shapes_mesh* par_shapes_create_cube();              // 6 sides polyhedron (cube)
par_shapes_mesh* par_shapes_create_octahedron();        // 8 sides polyhedron (dyamond)
par_shapes_mesh* par_shapes_create_dodecahedron();      // 12 sides polyhedron
par_shapes_mesh* par_shapes_create_icosahedron();       // 20 sides polyhedron
*/
    // Platonic solid generation: cube (6 sides)
    // NOTE: No normals/texcoords generated by default
    par_shapes_mesh *cube = par_shapes_create_cube();
    cube->tcoords = PAR_MALLOC(float, 2*cube->npoints);
    for (int i = 0; i < 2*cube->npoints; i++) cube->tcoords[i] = 0.0f;
    par_shapes_scale(cube, width, height, length);
    par_shapes_translate(cube, -width/2, 0.0f, -length/2);
    par_shapes_compute_normals(cube);

    mesh.vertices = (float *)malloc(cube->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(cube->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(cube->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = cube->ntriangles*3;
    mesh.triangleCount = cube->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = cube->points[cube->triangles[k]*3];
        mesh.vertices[k*3 + 1] = cube->points[cube->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = cube->points[cube->triangles[k]*3 + 2];

        mesh.normals[k*3] = cube->normals[cube->triangles[k]*3];
        mesh.normals[k*3 + 1] = cube->normals[cube->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = cube->normals[cube->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = cube->tcoords[cube->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = cube->tcoords[cube->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(cube);
#endif

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate sphere mesh (standard sphere)
RLAPI Mesh GenMeshSphere(float radius, int rings, int slices)
{
    Mesh mesh = { 0 };

    par_shapes_mesh *sphere = par_shapes_create_parametric_sphere(slices, rings);
    par_shapes_scale(sphere, radius, radius, radius);
    // NOTE: Soft normals are computed internally

    mesh.vertices = (float *)malloc(sphere->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(sphere->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(sphere->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = sphere->ntriangles*3;
    mesh.triangleCount = sphere->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = sphere->points[sphere->triangles[k]*3];
        mesh.vertices[k*3 + 1] = sphere->points[sphere->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = sphere->points[sphere->triangles[k]*3 + 2];

        mesh.normals[k*3] = sphere->normals[sphere->triangles[k]*3];
        mesh.normals[k*3 + 1] = sphere->normals[sphere->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = sphere->normals[sphere->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = sphere->tcoords[sphere->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = sphere->tcoords[sphere->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(sphere);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate hemi-sphere mesh (half sphere, no bottom cap)
RLAPI Mesh GenMeshHemiSphere(float radius, int rings, int slices)
{
    Mesh mesh = { 0 };

    par_shapes_mesh *sphere = par_shapes_create_hemisphere(slices, rings);
    par_shapes_scale(sphere, radius, radius, radius);
    // NOTE: Soft normals are computed internally

    mesh.vertices = (float *)malloc(sphere->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(sphere->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(sphere->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = sphere->ntriangles*3;
    mesh.triangleCount = sphere->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = sphere->points[sphere->triangles[k]*3];
        mesh.vertices[k*3 + 1] = sphere->points[sphere->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = sphere->points[sphere->triangles[k]*3 + 2];

        mesh.normals[k*3] = sphere->normals[sphere->triangles[k]*3];
        mesh.normals[k*3 + 1] = sphere->normals[sphere->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = sphere->normals[sphere->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = sphere->tcoords[sphere->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = sphere->tcoords[sphere->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(sphere);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate cylinder mesh
Mesh GenMeshCylinder(float radius, float height, int slices)
{
    Mesh mesh = { 0 };

    // Instance a cylinder that sits on the Z=0 plane using the given tessellation
    // levels across the UV domain.  Think of "slices" like a number of pizza
    // slices, and "stacks" like a number of stacked rings.
    // Height and radius are both 1.0, but they can easily be changed with par_shapes_scale
    par_shapes_mesh *cylinder = par_shapes_create_cylinder(slices, 8);
    par_shapes_scale(cylinder, radius, radius, height);
    par_shapes_rotate(cylinder, -PI/2.0f, (float[]){ 1, 0, 0 });

    // Generate an orientable disk shape (top cap)
    par_shapes_mesh *capTop = par_shapes_create_disk(radius, slices, (float[]){ 0, 0, 0 }, (float[]){ 0, 0, 1 });
    capTop->tcoords = PAR_MALLOC(float, 2*capTop->npoints);
    for (int i = 0; i < 2*capTop->npoints; i++) capTop->tcoords[i] = 0.0f;
    par_shapes_rotate(capTop, -PI/2.0f, (float[]){ 1, 0, 0 });
    par_shapes_translate(capTop, 0, height, 0);

    // Generate an orientable disk shape (bottom cap)
    par_shapes_mesh *capBottom = par_shapes_create_disk(radius, slices, (float[]){ 0, 0, 0 }, (float[]){ 0, 0, -1 });
    capBottom->tcoords = PAR_MALLOC(float, 2*capBottom->npoints);
    for (int i = 0; i < 2*capBottom->npoints; i++) capBottom->tcoords[i] = 0.95f;
    par_shapes_rotate(capBottom, PI/2.0f, (float[]){ 1, 0, 0 });

    par_shapes_merge_and_free(cylinder, capTop);
    par_shapes_merge_and_free(cylinder, capBottom);

    mesh.vertices = (float *)malloc(cylinder->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(cylinder->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(cylinder->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = cylinder->ntriangles*3;
    mesh.triangleCount = cylinder->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = cylinder->points[cylinder->triangles[k]*3];
        mesh.vertices[k*3 + 1] = cylinder->points[cylinder->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = cylinder->points[cylinder->triangles[k]*3 + 2];

        mesh.normals[k*3] = cylinder->normals[cylinder->triangles[k]*3];
        mesh.normals[k*3 + 1] = cylinder->normals[cylinder->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = cylinder->normals[cylinder->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = cylinder->tcoords[cylinder->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = cylinder->tcoords[cylinder->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(cylinder);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate torus mesh
Mesh GenMeshTorus(float radius, float size, int radSeg, int sides)
{
    Mesh mesh = { 0 };

    if (radius > 1.0f) radius = 1.0f;
    else if (radius < 0.1f) radius = 0.1f;

    // Create a donut that sits on the Z=0 plane with the specified inner radius
    // The outer radius can be controlled with par_shapes_scale
    par_shapes_mesh *torus = par_shapes_create_torus(radSeg, sides, radius);
    par_shapes_scale(torus, size/2, size/2, size/2);

    mesh.vertices = (float *)malloc(torus->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(torus->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(torus->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = torus->ntriangles*3;
    mesh.triangleCount = torus->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = torus->points[torus->triangles[k]*3];
        mesh.vertices[k*3 + 1] = torus->points[torus->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = torus->points[torus->triangles[k]*3 + 2];

        mesh.normals[k*3] = torus->normals[torus->triangles[k]*3];
        mesh.normals[k*3 + 1] = torus->normals[torus->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = torus->normals[torus->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = torus->tcoords[torus->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = torus->tcoords[torus->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(torus);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate trefoil knot mesh
Mesh GenMeshKnot(float radius, float size, int radSeg, int sides)
{
    Mesh mesh = { 0 };

    if (radius > 3.0f) radius = 3.0f;
    else if (radius < 0.5f) radius = 0.5f;

    par_shapes_mesh *knot = par_shapes_create_trefoil_knot(radSeg, sides, radius);
    par_shapes_scale(knot, size, size, size);

    mesh.vertices = (float *)malloc(knot->ntriangles*3*3*sizeof(float));
    mesh.texcoords = (float *)malloc(knot->ntriangles*3*2*sizeof(float));
    mesh.normals = (float *)malloc(knot->ntriangles*3*3*sizeof(float));

    mesh.vertexCount = knot->ntriangles*3;
    mesh.triangleCount = knot->ntriangles;

    for (int k = 0; k < mesh.vertexCount; k++)
    {
        mesh.vertices[k*3] = knot->points[knot->triangles[k]*3];
        mesh.vertices[k*3 + 1] = knot->points[knot->triangles[k]*3 + 1];
        mesh.vertices[k*3 + 2] = knot->points[knot->triangles[k]*3 + 2];

        mesh.normals[k*3] = knot->normals[knot->triangles[k]*3];
        mesh.normals[k*3 + 1] = knot->normals[knot->triangles[k]*3 + 1];
        mesh.normals[k*3 + 2] = knot->normals[knot->triangles[k]*3 + 2];

        mesh.texcoords[k*2] = knot->tcoords[knot->triangles[k]*2];
        mesh.texcoords[k*2 + 1] = knot->tcoords[knot->triangles[k]*2 + 1];
    }

    par_shapes_free_mesh(knot);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate a mesh from heightmap
// NOTE: Vertex data is uploaded to GPU
Mesh GenMeshHeightmap(Image heightmap, Vector3 size)
{
    #define GRAY_VALUE(c) ((c.r+c.g+c.b)/3)

    Mesh mesh = { 0 };

    int mapX = heightmap.width;
    int mapZ = heightmap.height;

    Color *pixels = GetImageData(heightmap);

    // NOTE: One vertex per pixel
    mesh.triangleCount = (mapX-1)*(mapZ-1)*2;    // One quad every four pixels

    mesh.vertexCount = mesh.triangleCount*3;

    mesh.vertices = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.normals = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)malloc(mesh.vertexCount*2*sizeof(float));
    mesh.colors = NULL;

    int vCounter = 0;       // Used to count vertices float by float
    int tcCounter = 0;      // Used to count texcoords float by float
    int nCounter = 0;       // Used to count normals float by float

    int trisCounter = 0;

    Vector3 scaleFactor = { size.x/mapX, size.y/255.0f, size.z/mapZ };

    for (int z = 0; z < mapZ-1; z++)
    {
        for (int x = 0; x < mapX-1; x++)
        {
            // Fill vertices array with data
            //----------------------------------------------------------

            // one triangle - 3 vertex
            mesh.vertices[vCounter] = (float)x*scaleFactor.x;
            mesh.vertices[vCounter + 1] = (float)GRAY_VALUE(pixels[x + z*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 2] = (float)z*scaleFactor.z;

            mesh.vertices[vCounter + 3] = (float)x*scaleFactor.x;
            mesh.vertices[vCounter + 4] = (float)GRAY_VALUE(pixels[x + (z + 1)*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 5] = (float)(z + 1)*scaleFactor.z;

            mesh.vertices[vCounter + 6] = (float)(x + 1)*scaleFactor.x;
            mesh.vertices[vCounter + 7] = (float)GRAY_VALUE(pixels[(x + 1) + z*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 8] = (float)z*scaleFactor.z;

            // another triangle - 3 vertex
            mesh.vertices[vCounter + 9] = mesh.vertices[vCounter + 6];
            mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
            mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

            mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
            mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
            mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

            mesh.vertices[vCounter + 15] = (float)(x + 1)*scaleFactor.x;
            mesh.vertices[vCounter + 16] = (float)GRAY_VALUE(pixels[(x + 1) + (z + 1)*mapX])*scaleFactor.y;
            mesh.vertices[vCounter + 17] = (float)(z + 1)*scaleFactor.z;
            vCounter += 18;     // 6 vertex, 18 floats

            // Fill texcoords array with data
            //--------------------------------------------------------------
            mesh.texcoords[tcCounter] = (float)x/(mapX - 1);
            mesh.texcoords[tcCounter + 1] = (float)z/(mapZ - 1);

            mesh.texcoords[tcCounter + 2] = (float)x/(mapX - 1);
            mesh.texcoords[tcCounter + 3] = (float)(z + 1)/(mapZ - 1);

            mesh.texcoords[tcCounter + 4] = (float)(x + 1)/(mapX - 1);
            mesh.texcoords[tcCounter + 5] = (float)z/(mapZ - 1);

            mesh.texcoords[tcCounter + 6] = mesh.texcoords[tcCounter + 4];
            mesh.texcoords[tcCounter + 7] = mesh.texcoords[tcCounter + 5];

            mesh.texcoords[tcCounter + 8] = mesh.texcoords[tcCounter + 2];
            mesh.texcoords[tcCounter + 9] = mesh.texcoords[tcCounter + 3];

            mesh.texcoords[tcCounter + 10] = (float)(x + 1)/(mapX - 1);
            mesh.texcoords[tcCounter + 11] = (float)(z + 1)/(mapZ - 1);
            tcCounter += 12;    // 6 texcoords, 12 floats

            // Fill normals array with data
            //--------------------------------------------------------------
            for (int i = 0; i < 18; i += 3)
            {
                mesh.normals[nCounter + i] = 0.0f;
                mesh.normals[nCounter + i + 1] = 1.0f;
                mesh.normals[nCounter + i + 2] = 0.0f;
            }

            // TODO: Calculate normals in an efficient way

            nCounter += 18;     // 6 vertex, 18 floats
            trisCounter += 2;
        }
    }

    free(pixels);

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}

// Generate a cubes mesh from pixel data
// NOTE: Vertex data is uploaded to GPU
Mesh GenMeshCubicmap(Image cubicmap, Vector3 cubeSize)
{
    Mesh mesh = { 0 };

    Color *cubicmapPixels = GetImageData(cubicmap);

    int mapWidth = cubicmap.width;
    int mapHeight = cubicmap.height;

    // NOTE: Max possible number of triangles numCubes * (12 triangles by cube)
    int maxTriangles = cubicmap.width*cubicmap.height*12;

    int vCounter = 0;       // Used to count vertices
    int tcCounter = 0;      // Used to count texcoords
    int nCounter = 0;       // Used to count normals

    float w = cubeSize.x;
    float h = cubeSize.z;
    float h2 = cubeSize.y;

    Vector3 *mapVertices = (Vector3 *)malloc(maxTriangles*3*sizeof(Vector3));
    Vector2 *mapTexcoords = (Vector2 *)malloc(maxTriangles*3*sizeof(Vector2));
    Vector3 *mapNormals = (Vector3 *)malloc(maxTriangles*3*sizeof(Vector3));

    // Define the 6 normals of the cube, we will combine them accordingly later...
    Vector3 n1 = { 1.0f, 0.0f, 0.0f };
    Vector3 n2 = { -1.0f, 0.0f, 0.0f };
    Vector3 n3 = { 0.0f, 1.0f, 0.0f };
    Vector3 n4 = { 0.0f, -1.0f, 0.0f };
    Vector3 n5 = { 0.0f, 0.0f, 1.0f };
    Vector3 n6 = { 0.0f, 0.0f, -1.0f };

    // NOTE: We use texture rectangles to define different textures for top-bottom-front-back-right-left (6)
    typedef struct RectangleF {
        float x;
        float y;
        float width;
        float height;
    } RectangleF;

    RectangleF rightTexUV = { 0.0f, 0.0f, 0.5f, 0.5f };
    RectangleF leftTexUV = { 0.5f, 0.0f, 0.5f, 0.5f };
    RectangleF frontTexUV = { 0.0f, 0.0f, 0.5f, 0.5f };
    RectangleF backTexUV = { 0.5f, 0.0f, 0.5f, 0.5f };
    RectangleF topTexUV = { 0.0f, 0.5f, 0.5f, 0.5f };
    RectangleF bottomTexUV = { 0.5f, 0.5f, 0.5f, 0.5f };

    for (int z = 0; z < mapHeight; ++z)
    {
        for (int x = 0; x < mapWidth; ++x)
        {
            // Define the 8 vertex of the cube, we will combine them accordingly later...
            Vector3 v1 = { w*(x - 0.5f), h2, h*(z - 0.5f) };
            Vector3 v2 = { w*(x - 0.5f), h2, h*(z + 0.5f) };
            Vector3 v3 = { w*(x + 0.5f), h2, h*(z + 0.5f) };
            Vector3 v4 = { w*(x + 0.5f), h2, h*(z - 0.5f) };
            Vector3 v5 = { w*(x + 0.5f), 0, h*(z - 0.5f) };
            Vector3 v6 = { w*(x - 0.5f), 0, h*(z - 0.5f) };
            Vector3 v7 = { w*(x - 0.5f), 0, h*(z + 0.5f) };
            Vector3 v8 = { w*(x + 0.5f), 0, h*(z + 0.5f) };

            // We check pixel color to be WHITE, we will full cubes
            if ((cubicmapPixels[z*cubicmap.width + x].r == 255) &&
                (cubicmapPixels[z*cubicmap.width + x].g == 255) &&
                (cubicmapPixels[z*cubicmap.width + x].b == 255))
            {
                // Define triangles (Checking Collateral Cubes!)
                //----------------------------------------------

                // Define top triangles (2 tris, 6 vertex --> v1-v2-v3, v1-v3-v4)
                mapVertices[vCounter] = v1;
                mapVertices[vCounter + 1] = v2;
                mapVertices[vCounter + 2] = v3;
                mapVertices[vCounter + 3] = v1;
                mapVertices[vCounter + 4] = v3;
                mapVertices[vCounter + 5] = v4;
                vCounter += 6;

                mapNormals[nCounter] = n3;
                mapNormals[nCounter + 1] = n3;
                mapNormals[nCounter + 2] = n3;
                mapNormals[nCounter + 3] = n3;
                mapNormals[nCounter + 4] = n3;
                mapNormals[nCounter + 5] = n3;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ topTexUV.x, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 5] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y };
                tcCounter += 6;

                // Define bottom triangles (2 tris, 6 vertex --> v6-v8-v7, v6-v5-v8)
                mapVertices[vCounter] = v6;
                mapVertices[vCounter + 1] = v8;
                mapVertices[vCounter + 2] = v7;
                mapVertices[vCounter + 3] = v6;
                mapVertices[vCounter + 4] = v5;
                mapVertices[vCounter + 5] = v8;
                vCounter += 6;

                mapNormals[nCounter] = n4;
                mapNormals[nCounter + 1] = n4;
                mapNormals[nCounter + 2] = n4;
                mapNormals[nCounter + 3] = n4;
                mapNormals[nCounter + 4] = n4;
                mapNormals[nCounter + 5] = n4;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ bottomTexUV.x, bottomTexUV.y };
                mapTexcoords[tcCounter + 5] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                tcCounter += 6;

                if (((z < cubicmap.height - 1) &&
                (cubicmapPixels[(z + 1)*cubicmap.width + x].r == 0) &&
                (cubicmapPixels[(z + 1)*cubicmap.width + x].g == 0) &&
                (cubicmapPixels[(z + 1)*cubicmap.width + x].b == 0)) || (z == cubicmap.height - 1))
                {
                    // Define front triangles (2 tris, 6 vertex) --> v2 v7 v3, v3 v7 v8
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v2;
                    mapVertices[vCounter + 1] = v7;
                    mapVertices[vCounter + 2] = v3;
                    mapVertices[vCounter + 3] = v3;
                    mapVertices[vCounter + 4] = v7;
                    mapVertices[vCounter + 5] = v8;
                    vCounter += 6;

                    mapNormals[nCounter] = n6;
                    mapNormals[nCounter + 1] = n6;
                    mapNormals[nCounter + 2] = n6;
                    mapNormals[nCounter + 3] = n6;
                    mapNormals[nCounter + 4] = n6;
                    mapNormals[nCounter + 5] = n6;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ frontTexUV.x, frontTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ frontTexUV.x, frontTexUV.y + frontTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ frontTexUV.x + frontTexUV.width, frontTexUV.y };
                    mapTexcoords[tcCounter + 3] = (Vector2){ frontTexUV.x + frontTexUV.width, frontTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ frontTexUV.x, frontTexUV.y + frontTexUV.height };
                    mapTexcoords[tcCounter + 5] = (Vector2){ frontTexUV.x + frontTexUV.width, frontTexUV.y + frontTexUV.height };
                    tcCounter += 6;
                }

                if (((z > 0) &&
                (cubicmapPixels[(z - 1)*cubicmap.width + x].r == 0) &&
                (cubicmapPixels[(z - 1)*cubicmap.width + x].g == 0) &&
                (cubicmapPixels[(z - 1)*cubicmap.width + x].b == 0)) || (z == 0))
                {
                    // Define back triangles (2 tris, 6 vertex) --> v1 v5 v6, v1 v4 v5
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v1;
                    mapVertices[vCounter + 1] = v5;
                    mapVertices[vCounter + 2] = v6;
                    mapVertices[vCounter + 3] = v1;
                    mapVertices[vCounter + 4] = v4;
                    mapVertices[vCounter + 5] = v5;
                    vCounter += 6;

                    mapNormals[nCounter] = n5;
                    mapNormals[nCounter + 1] = n5;
                    mapNormals[nCounter + 2] = n5;
                    mapNormals[nCounter + 3] = n5;
                    mapNormals[nCounter + 4] = n5;
                    mapNormals[nCounter + 5] = n5;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ backTexUV.x + backTexUV.width, backTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ backTexUV.x, backTexUV.y + backTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ backTexUV.x + backTexUV.width, backTexUV.y + backTexUV.height };
                    mapTexcoords[tcCounter + 3] = (Vector2){ backTexUV.x + backTexUV.width, backTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ backTexUV.x, backTexUV.y };
                    mapTexcoords[tcCounter + 5] = (Vector2){ backTexUV.x, backTexUV.y + backTexUV.height };
                    tcCounter += 6;
                }

                if (((x < cubicmap.width - 1) &&
                (cubicmapPixels[z*cubicmap.width + (x + 1)].r == 0) &&
                (cubicmapPixels[z*cubicmap.width + (x + 1)].g == 0) &&
                (cubicmapPixels[z*cubicmap.width + (x + 1)].b == 0)) || (x == cubicmap.width - 1))
                {
                    // Define right triangles (2 tris, 6 vertex) --> v3 v8 v4, v4 v8 v5
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v3;
                    mapVertices[vCounter + 1] = v8;
                    mapVertices[vCounter + 2] = v4;
                    mapVertices[vCounter + 3] = v4;
                    mapVertices[vCounter + 4] = v8;
                    mapVertices[vCounter + 5] = v5;
                    vCounter += 6;

                    mapNormals[nCounter] = n1;
                    mapNormals[nCounter + 1] = n1;
                    mapNormals[nCounter + 2] = n1;
                    mapNormals[nCounter + 3] = n1;
                    mapNormals[nCounter + 4] = n1;
                    mapNormals[nCounter + 5] = n1;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ rightTexUV.x, rightTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ rightTexUV.x, rightTexUV.y + rightTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ rightTexUV.x + rightTexUV.width, rightTexUV.y };
                    mapTexcoords[tcCounter + 3] = (Vector2){ rightTexUV.x + rightTexUV.width, rightTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ rightTexUV.x, rightTexUV.y + rightTexUV.height };
                    mapTexcoords[tcCounter + 5] = (Vector2){ rightTexUV.x + rightTexUV.width, rightTexUV.y + rightTexUV.height };
                    tcCounter += 6;
                }

                if (((x > 0) &&
                (cubicmapPixels[z*cubicmap.width + (x - 1)].r == 0) &&
                (cubicmapPixels[z*cubicmap.width + (x - 1)].g == 0) &&
                (cubicmapPixels[z*cubicmap.width + (x - 1)].b == 0)) || (x == 0))
                {
                    // Define left triangles (2 tris, 6 vertex) --> v1 v7 v2, v1 v6 v7
                    // NOTE: Collateral occluded faces are not generated
                    mapVertices[vCounter] = v1;
                    mapVertices[vCounter + 1] = v7;
                    mapVertices[vCounter + 2] = v2;
                    mapVertices[vCounter + 3] = v1;
                    mapVertices[vCounter + 4] = v6;
                    mapVertices[vCounter + 5] = v7;
                    vCounter += 6;

                    mapNormals[nCounter] = n2;
                    mapNormals[nCounter + 1] = n2;
                    mapNormals[nCounter + 2] = n2;
                    mapNormals[nCounter + 3] = n2;
                    mapNormals[nCounter + 4] = n2;
                    mapNormals[nCounter + 5] = n2;
                    nCounter += 6;

                    mapTexcoords[tcCounter] = (Vector2){ leftTexUV.x, leftTexUV.y };
                    mapTexcoords[tcCounter + 1] = (Vector2){ leftTexUV.x + leftTexUV.width, leftTexUV.y + leftTexUV.height };
                    mapTexcoords[tcCounter + 2] = (Vector2){ leftTexUV.x + leftTexUV.width, leftTexUV.y };
                    mapTexcoords[tcCounter + 3] = (Vector2){ leftTexUV.x, leftTexUV.y };
                    mapTexcoords[tcCounter + 4] = (Vector2){ leftTexUV.x, leftTexUV.y + leftTexUV.height };
                    mapTexcoords[tcCounter + 5] = (Vector2){ leftTexUV.x + leftTexUV.width, leftTexUV.y + leftTexUV.height };
                    tcCounter += 6;
                }
            }
            // We check pixel color to be BLACK, we will only draw floor and roof
            else if  ((cubicmapPixels[z*cubicmap.width + x].r == 0) &&
                      (cubicmapPixels[z*cubicmap.width + x].g == 0) &&
                      (cubicmapPixels[z*cubicmap.width + x].b == 0))
            {
                // Define top triangles (2 tris, 6 vertex --> v1-v2-v3, v1-v3-v4)
                mapVertices[vCounter] = v1;
                mapVertices[vCounter + 1] = v3;
                mapVertices[vCounter + 2] = v2;
                mapVertices[vCounter + 3] = v1;
                mapVertices[vCounter + 4] = v4;
                mapVertices[vCounter + 5] = v3;
                vCounter += 6;

                mapNormals[nCounter] = n4;
                mapNormals[nCounter + 1] = n4;
                mapNormals[nCounter + 2] = n4;
                mapNormals[nCounter + 3] = n4;
                mapNormals[nCounter + 4] = n4;
                mapNormals[nCounter + 5] = n4;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ topTexUV.x, topTexUV.y + topTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ topTexUV.x, topTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y };
                mapTexcoords[tcCounter + 5] = (Vector2){ topTexUV.x + topTexUV.width, topTexUV.y + topTexUV.height };
                tcCounter += 6;

                // Define bottom triangles (2 tris, 6 vertex --> v6-v8-v7, v6-v5-v8)
                mapVertices[vCounter] = v6;
                mapVertices[vCounter + 1] = v7;
                mapVertices[vCounter + 2] = v8;
                mapVertices[vCounter + 3] = v6;
                mapVertices[vCounter + 4] = v8;
                mapVertices[vCounter + 5] = v5;
                vCounter += 6;

                mapNormals[nCounter] = n3;
                mapNormals[nCounter + 1] = n3;
                mapNormals[nCounter + 2] = n3;
                mapNormals[nCounter + 3] = n3;
                mapNormals[nCounter + 4] = n3;
                mapNormals[nCounter + 5] = n3;
                nCounter += 6;

                mapTexcoords[tcCounter] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 1] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 2] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 3] = (Vector2){ bottomTexUV.x + bottomTexUV.width, bottomTexUV.y };
                mapTexcoords[tcCounter + 4] = (Vector2){ bottomTexUV.x, bottomTexUV.y + bottomTexUV.height };
                mapTexcoords[tcCounter + 5] = (Vector2){ bottomTexUV.x, bottomTexUV.y };
                tcCounter += 6;
            }
        }
    }

    // Move data from mapVertices temp arays to vertices float array
    mesh.vertexCount = vCounter;
    mesh.triangleCount = vCounter/3;

    mesh.vertices = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.normals = (float *)malloc(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)malloc(mesh.vertexCount*2*sizeof(float));
    mesh.colors = NULL;

    int fCounter = 0;

    // Move vertices data
    for (int i = 0; i < vCounter; i++)
    {
        mesh.vertices[fCounter] = mapVertices[i].x;
        mesh.vertices[fCounter + 1] = mapVertices[i].y;
        mesh.vertices[fCounter + 2] = mapVertices[i].z;
        fCounter += 3;
    }

    fCounter = 0;

    // Move normals data
    for (int i = 0; i < nCounter; i++)
    {
        mesh.normals[fCounter] = mapNormals[i].x;
        mesh.normals[fCounter + 1] = mapNormals[i].y;
        mesh.normals[fCounter + 2] = mapNormals[i].z;
        fCounter += 3;
    }

    fCounter = 0;

    // Move texcoords data
    for (int i = 0; i < tcCounter; i++)
    {
        mesh.texcoords[fCounter] = mapTexcoords[i].x;
        mesh.texcoords[fCounter + 1] = mapTexcoords[i].y;
        fCounter += 2;
    }

    free(mapVertices);
    free(mapNormals);
    free(mapTexcoords);

    free(cubicmapPixels);   // Free image pixel data

    // Upload vertex data to GPU (static mesh)
    rlLoadMesh(&mesh, false);

    return mesh;
}
#endif      // SUPPORT_MESH_GENERATION

// Load material data (from file)
Material LoadMaterial(const char *fileName)
{
    Material material = { 0 };

#if defined(SUPPORT_FILEFORMAT_MTL)
    if (IsFileExtension(fileName, ".mtl"))
    {
        tinyobj_material_t *materials;
        unsigned int materialCount = 0;

        int result = tinyobj_parse_mtl_file(&materials, &materialCount, fileName);

        // TODO: Process materials to return

        tinyobj_materials_free(materials, materialCount);
    }
#else
    TraceLog(LOG_WARNING, "[%s] Material fileformat not supported, it can't be loaded", fileName);
#endif

    // Our material uses the default shader (DIFFUSE, SPECULAR, NORMAL)
    material.shader = GetShaderDefault();

    return material;
}

// Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps)
Material LoadMaterialDefault(void)
{
    Material material = { 0 };

    material.shader = GetShaderDefault();
    material.maps[MAP_DIFFUSE].texture = GetTextureDefault();   // White texture (1x1 pixel)
    //material.maps[MAP_NORMAL].texture;         // NOTE: By default, not set
    //material.maps[MAP_SPECULAR].texture;       // NOTE: By default, not set

    material.maps[MAP_DIFFUSE].color = WHITE;    // Diffuse color
    material.maps[MAP_SPECULAR].color = WHITE;   // Specular color

    return material;
}

// Unload material from memory
void UnloadMaterial(Material material)
{
    // Unload material shader (avoid unloading default shader, managed by raylib)
    if (material.shader.id != GetShaderDefault().id) UnloadShader(material.shader);

    // Unload loaded texture maps (avoid unloading default texture, managed by raylib)
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++)
    {
        if (material.maps[i].texture.id != GetTextureDefault().id) rlDeleteTextures(material.maps[i].texture.id);
    }
}

// Draw a model (with texture if set)
void DrawModel(Model model, Vector3 position, float scale, Color tint)
{
    Vector3 vScale = { scale, scale, scale };
    Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };

    DrawModelEx(model, position, rotationAxis, 0.0f, vScale, tint);
}

// Draw a model with extended parameters
void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint)
{
    // Calculate transformation matrix from function parameters
    // Get transform matrix (rotation -> scale -> translation)
    Matrix matScale = MatrixScale(scale.x, scale.y, scale.z);
    Matrix matRotation = MatrixRotate(rotationAxis, rotationAngle*DEG2RAD);
    Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);

    Matrix matTransform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

    // Combine model transformation matrix (model.transform) with matrix generated by function parameters (matTransform)
    model.transform = MatrixMultiply(model.transform, matTransform);

    for (int i = 0; i < model.meshCount; i++)
    {
        model.materials[model.meshMaterial[i]].maps[MAP_DIFFUSE].color = tint;
        rlDrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], model.transform);
    }
}

// Draw a model wires (with texture if set)
void DrawModelWires(Model model, Vector3 position, float scale, Color tint)
{
    rlEnableWireMode();

    DrawModel(model, position, scale, tint);

    rlDisableWireMode();
}

// Draw a model wires (with texture if set) with extended parameters
void DrawModelWiresEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint)
{
    rlEnableWireMode();

    DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);

    rlDisableWireMode();
}

// Draw a billboard
void DrawBillboard(Camera camera, Texture2D texture, Vector3 center, float size, Color tint)
{
    Rectangle sourceRec = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };

    DrawBillboardRec(camera, texture, sourceRec, center, size, tint);
}

// Draw a billboard (part of a texture defined by a rectangle)
void DrawBillboardRec(Camera camera, Texture2D texture, Rectangle sourceRec, Vector3 center, float size, Color tint)
{
    // NOTE: Billboard size will maintain sourceRec aspect ratio, size will represent billboard width
    Vector2 sizeRatio = { size, size*(float)sourceRec.height/sourceRec.width };

    Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);

    Vector3 right = { matView.m0, matView.m4, matView.m8 };
    //Vector3 up = { matView.m1, matView.m5, matView.m9 };

    // NOTE: Billboard locked on axis-Y
    Vector3 up = { 0.0f, 1.0f, 0.0f };
/*
    a-------b
    |       |
    |   *   |
    |       |
    d-------c
*/
    right = Vector3Scale(right, sizeRatio.x/2);
    up = Vector3Scale(up, sizeRatio.y/2);

    Vector3 p1 = Vector3Add(right, up);
    Vector3 p2 = Vector3Subtract(right, up);

    Vector3 a = Vector3Subtract(center, p2);
    Vector3 b = Vector3Add(center, p1);
    Vector3 c = Vector3Add(center, p2);
    Vector3 d = Vector3Subtract(center, p1);

    rlEnableTexture(texture.id);

    rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);

        // Bottom-left corner for texture and quad
        rlTexCoord2f((float)sourceRec.x/texture.width, (float)sourceRec.y/texture.height);
        rlVertex3f(a.x, a.y, a.z);

        // Top-left corner for texture and quad
        rlTexCoord2f((float)sourceRec.x/texture.width, (float)(sourceRec.y + sourceRec.height)/texture.height);
        rlVertex3f(d.x, d.y, d.z);

        // Top-right corner for texture and quad
        rlTexCoord2f((float)(sourceRec.x + sourceRec.width)/texture.width, (float)(sourceRec.y + sourceRec.height)/texture.height);
        rlVertex3f(c.x, c.y, c.z);

        // Bottom-right corner for texture and quad
        rlTexCoord2f((float)(sourceRec.x + sourceRec.width)/texture.width, (float)sourceRec.y/texture.height);
        rlVertex3f(b.x, b.y, b.z);
    rlEnd();

    rlDisableTexture();
}

// Draw a bounding box with wires
void DrawBoundingBox(BoundingBox box, Color color)
{
    Vector3 size;

    size.x = (float)fabs(box.max.x - box.min.x);
    size.y = (float)fabs(box.max.y - box.min.y);
    size.z = (float)fabs(box.max.z - box.min.z);

    Vector3 center = { box.min.x + size.x/2.0f, box.min.y + size.y/2.0f, box.min.z + size.z/2.0f };

    DrawCubeWires(center, size.x, size.y, size.z, color);
}

// Detect collision between two spheres
bool CheckCollisionSpheres(Vector3 centerA, float radiusA, Vector3 centerB, float radiusB)
{
    bool collision = false;

    float dx = centerA.x - centerB.x;      // X distance between centers
    float dy = centerA.y - centerB.y;      // Y distance between centers
    float dz = centerA.z - centerB.z;      // Y distance between centers

    float distance = sqrtf(dx*dx + dy*dy + dz*dz);  // Distance between centers

    if (distance <= (radiusA + radiusB)) collision = true;

    return collision;
}

// Detect collision between two boxes
// NOTE: Boxes are defined by two points minimum and maximum
bool CheckCollisionBoxes(BoundingBox box1, BoundingBox box2)
{
    bool collision = true;

    if ((box1.max.x >= box2.min.x) && (box1.min.x <= box2.max.x))
    {
        if ((box1.max.y < box2.min.y) || (box1.min.y > box2.max.y)) collision = false;
        if ((box1.max.z < box2.min.z) || (box1.min.z > box2.max.z)) collision = false;
    }
    else collision = false;

    return collision;
}

// Detect collision between box and sphere
bool CheckCollisionBoxSphere(BoundingBox box, Vector3 centerSphere, float radiusSphere)
{
    bool collision = false;

    float dmin = 0;

    if (centerSphere.x < box.min.x) dmin += powf(centerSphere.x - box.min.x, 2);
    else if (centerSphere.x > box.max.x) dmin += powf(centerSphere.x - box.max.x, 2);

    if (centerSphere.y < box.min.y) dmin += powf(centerSphere.y - box.min.y, 2);
    else if (centerSphere.y > box.max.y) dmin += powf(centerSphere.y - box.max.y, 2);

    if (centerSphere.z < box.min.z) dmin += powf(centerSphere.z - box.min.z, 2);
    else if (centerSphere.z > box.max.z) dmin += powf(centerSphere.z - box.max.z, 2);

    if (dmin <= (radiusSphere*radiusSphere)) collision = true;

    return collision;
}

// Detect collision between ray and sphere
bool CheckCollisionRaySphere(Ray ray, Vector3 spherePosition, float sphereRadius)
{
    bool collision = false;

    Vector3 raySpherePos = Vector3Subtract(spherePosition, ray.position);
    float distance = Vector3Length(raySpherePos);
    float vector = Vector3DotProduct(raySpherePos, ray.direction);
    float d = sphereRadius*sphereRadius - (distance*distance - vector*vector);

    if (d >= 0.0f) collision = true;

    return collision;
}

// Detect collision between ray and sphere with extended parameters and collision point detection
bool CheckCollisionRaySphereEx(Ray ray, Vector3 spherePosition, float sphereRadius, Vector3 *collisionPoint)
{
    bool collision = false;

    Vector3 raySpherePos = Vector3Subtract(spherePosition, ray.position);
    float distance = Vector3Length(raySpherePos);
    float vector = Vector3DotProduct(raySpherePos, ray.direction);
    float d = sphereRadius*sphereRadius - (distance*distance - vector*vector);

    if (d >= 0.0f) collision = true;

    // Check if ray origin is inside the sphere to calculate the correct collision point
    float collisionDistance = 0;

    if (distance < sphereRadius) collisionDistance = vector + sqrtf(d);
    else collisionDistance = vector - sqrtf(d);

    // Calculate collision point
    Vector3 cPoint = Vector3Add(ray.position, Vector3Scale(ray.direction, collisionDistance));

    collisionPoint->x = cPoint.x;
    collisionPoint->y = cPoint.y;
    collisionPoint->z = cPoint.z;

    return collision;
}

// Detect collision between ray and bounding box
bool CheckCollisionRayBox(Ray ray, BoundingBox box)
{
    bool collision = false;

    float t[8];
    t[0] = (box.min.x - ray.position.x)/ray.direction.x;
    t[1] = (box.max.x - ray.position.x)/ray.direction.x;
    t[2] = (box.min.y - ray.position.y)/ray.direction.y;
    t[3] = (box.max.y - ray.position.y)/ray.direction.y;
    t[4] = (box.min.z - ray.position.z)/ray.direction.z;
    t[5] = (box.max.z - ray.position.z)/ray.direction.z;
    t[6] = (float)fmax(fmax(fmin(t[0], t[1]), fmin(t[2], t[3])), fmin(t[4], t[5]));
    t[7] = (float)fmin(fmin(fmax(t[0], t[1]), fmax(t[2], t[3])), fmax(t[4], t[5]));

    collision = !(t[7] < 0 || t[6] > t[7]);

    return collision;
}

// Get collision info between ray and model
RayHitInfo GetCollisionRayModel(Ray ray, Model *model)
{
    RayHitInfo result = { 0 };

    for (int m = 0; m < model->meshCount; m++)
    {
        // Check if meshhas vertex data on CPU for testing
        if (model->meshes[m].vertices != NULL)
        {
            // model->mesh.triangleCount may not be set, vertexCount is more reliable
            int triangleCount = model->meshes[m].vertexCount/3;

            // Test against all triangles in mesh
            for (int i = 0; i < triangleCount; i++)
            {
                Vector3 a, b, c;
                Vector3 *vertdata = (Vector3 *)model->meshes[m].vertices;

                if (model->meshes[m].indices)
                {
                    a = vertdata[model->meshes[m].indices[i*3 + 0]];
                    b = vertdata[model->meshes[m].indices[i*3 + 1]];
                    c = vertdata[model->meshes[m].indices[i*3 + 2]];
                }
                else
                {
                    a = vertdata[i*3 + 0];
                    b = vertdata[i*3 + 1];
                    c = vertdata[i*3 + 2];
                }

                a = Vector3Transform(a, model->transform);
                b = Vector3Transform(b, model->transform);
                c = Vector3Transform(c, model->transform);

                RayHitInfo triHitInfo = GetCollisionRayTriangle(ray, a, b, c);

                if (triHitInfo.hit)
                {
                    // Save the closest hit triangle
                    if ((!result.hit) || (result.distance > triHitInfo.distance)) result = triHitInfo;
                }
            }
        }
    }

    return result;
}

// Get collision info between ray and triangle
// NOTE: Based on https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
RayHitInfo GetCollisionRayTriangle(Ray ray, Vector3 p1, Vector3 p2, Vector3 p3)
{
    #define EPSILON 0.000001        // A small number

    Vector3 edge1, edge2;
    Vector3 p, q, tv;
    float det, invDet, u, v, t;
    RayHitInfo result = {0};

    // Find vectors for two edges sharing V1
    edge1 = Vector3Subtract(p2, p1);
    edge2 = Vector3Subtract(p3, p1);

    // Begin calculating determinant - also used to calculate u parameter
    p = Vector3CrossProduct(ray.direction, edge2);

    // If determinant is near zero, ray lies in plane of triangle or ray is parallel to plane of triangle
    det = Vector3DotProduct(edge1, p);

    // Avoid culling!
    if ((det > -EPSILON) && (det < EPSILON)) return result;

    invDet = 1.0f/det;

    // Calculate distance from V1 to ray origin
    tv = Vector3Subtract(ray.position, p1);

    // Calculate u parameter and test bound
    u = Vector3DotProduct(tv, p)*invDet;

    // The intersection lies outside of the triangle
    if ((u < 0.0f) || (u > 1.0f)) return result;

    // Prepare to test v parameter
    q = Vector3CrossProduct(tv, edge1);

    // Calculate V parameter and test bound
    v = Vector3DotProduct(ray.direction, q)*invDet;

    // The intersection lies outside of the triangle
    if ((v < 0.0f) || ((u + v) > 1.0f)) return result;

    t = Vector3DotProduct(edge2, q)*invDet;

    if (t > EPSILON)
    {
        // Ray hit, get hit point and normal
        result.hit = true;
        result.distance = t;
        result.hit = true;
        result.normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));
        result.position = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }

    return result;
}

// Get collision info between ray and ground plane (Y-normal plane)
RayHitInfo GetCollisionRayGround(Ray ray, float groundHeight)
{
    #define EPSILON 0.000001        // A small number

    RayHitInfo result = { 0 };

    if (fabs(ray.direction.y) > EPSILON)
    {
        float distance = (ray.position.y - groundHeight)/-ray.direction.y;

        if (distance >= 0.0)
        {
            result.hit = true;
            result.distance = distance;
            result.normal = (Vector3){ 0.0, 1.0, 0.0 };
            result.position = Vector3Add(ray.position, Vector3Scale(ray.direction, distance));
        }
    }

    return result;
}

// Compute mesh bounding box limits
// NOTE: minVertex and maxVertex should be transformed by model transform matrix
BoundingBox MeshBoundingBox(Mesh mesh)
{
    // Get min and max vertex to construct bounds (AABB)
    Vector3 minVertex = { 0 };
    Vector3 maxVertex = { 0 };

    printf("Mesh vertex count: %i\n", mesh.vertexCount);

    if (mesh.vertices != NULL)
    {
        minVertex = (Vector3){ mesh.vertices[0], mesh.vertices[1], mesh.vertices[2] };
        maxVertex = (Vector3){ mesh.vertices[0], mesh.vertices[1], mesh.vertices[2] };

        for (int i = 1; i < mesh.vertexCount; i++)
        {
            minVertex = Vector3Min(minVertex, (Vector3){ mesh.vertices[i*3], mesh.vertices[i*3 + 1], mesh.vertices[i*3 + 2] });
            maxVertex = Vector3Max(maxVertex, (Vector3){ mesh.vertices[i*3], mesh.vertices[i*3 + 1], mesh.vertices[i*3 + 2] });
        }
    }

    // Create the bounding box
    BoundingBox box = { 0 };
    box.min = minVertex;
    box.max = maxVertex;

    return box;
}

// Compute mesh tangents
// NOTE: To calculate mesh tangents and binormals we need mesh vertex positions and texture coordinates
// Implementation base don: https://answers.unity.com/questions/7789/calculating-tangents-vector4.html
void MeshTangents(Mesh *mesh)
{
    if (mesh->tangents == NULL) mesh->tangents = (float *)malloc(mesh->vertexCount*4*sizeof(float));
    else TraceLog(LOG_WARNING, "Mesh tangents already exist");

    Vector3 *tan1 = (Vector3 *)malloc(mesh->vertexCount*sizeof(Vector3));
    Vector3 *tan2 = (Vector3 *)malloc(mesh->vertexCount*sizeof(Vector3));

    for (int i = 0; i < mesh->vertexCount; i += 3)
    {
        // Get triangle vertices
        Vector3 v1 = { mesh->vertices[(i + 0)*3 + 0], mesh->vertices[(i + 0)*3 + 1], mesh->vertices[(i + 0)*3 + 2] };
        Vector3 v2 = { mesh->vertices[(i + 1)*3 + 0], mesh->vertices[(i + 1)*3 + 1], mesh->vertices[(i + 1)*3 + 2] };
        Vector3 v3 = { mesh->vertices[(i + 2)*3 + 0], mesh->vertices[(i + 2)*3 + 1], mesh->vertices[(i + 2)*3 + 2] };

        // Get triangle texcoords
        Vector2 uv1 = { mesh->texcoords[(i + 0)*2 + 0], mesh->texcoords[(i + 0)*2 + 1] };
        Vector2 uv2 = { mesh->texcoords[(i + 1)*2 + 0], mesh->texcoords[(i + 1)*2 + 1] };
        Vector2 uv3 = { mesh->texcoords[(i + 2)*2 + 0], mesh->texcoords[(i + 2)*2 + 1] };

        float x1 = v2.x - v1.x;
        float y1 = v2.y - v1.y;
        float z1 = v2.z - v1.z;
        float x2 = v3.x - v1.x;
        float y2 = v3.y - v1.y;
        float z2 = v3.z - v1.z;

        float s1 = uv2.x - uv1.x;
        float t1 = uv2.y - uv1.y;
        float s2 = uv3.x - uv1.x;
        float t2 = uv3.y - uv1.y;

        float div = s1*t2 - s2*t1;
        float r = (div == 0.0f)? 0.0f : 1.0f/div;

        Vector3 sdir = { (t2*x1 - t1*x2)*r, (t2*y1 - t1*y2)*r, (t2*z1 - t1*z2)*r };
        Vector3 tdir = { (s1*x2 - s2*x1)*r, (s1*y2 - s2*y1)*r, (s1*z2 - s2*z1)*r };

        tan1[i + 0] = sdir;
        tan1[i + 1] = sdir;
        tan1[i + 2] = sdir;

        tan2[i + 0] = tdir;
        tan2[i + 1] = tdir;
        tan2[i + 2] = tdir;
    }

    // Compute tangents considering normals
    for (int i = 0; i < mesh->vertexCount; ++i)
    {
        Vector3 normal = { mesh->normals[i*3 + 0], mesh->normals[i*3 + 1], mesh->normals[i*3 + 2] };
        Vector3 tangent = tan1[i];

        // TODO: Review, not sure if tangent computation is right, just used reference proposed maths...
    #if defined(COMPUTE_TANGENTS_METHOD_01)
        Vector3 tmp = Vector3Subtract(tangent, Vector3Multiply(normal, Vector3DotProduct(normal, tangent)));
        tmp = Vector3Normalize(tmp);
        mesh->tangents[i*4 + 0] = tmp.x;
        mesh->tangents[i*4 + 1] = tmp.y;
        mesh->tangents[i*4 + 2] = tmp.z;
        mesh->tangents[i*4 + 3] = 1.0f;
    #else
        Vector3OrthoNormalize(&normal, &tangent);
        mesh->tangents[i*4 + 0] = tangent.x;
        mesh->tangents[i*4 + 1] = tangent.y;
        mesh->tangents[i*4 + 2] = tangent.z;
        mesh->tangents[i*4 + 3] = (Vector3DotProduct(Vector3CrossProduct(normal, tangent), tan2[i]) < 0.0f)? -1.0f : 1.0f;
    #endif
    }

    free(tan1);
    free(tan2);

    TraceLog(LOG_INFO, "Tangents computed for mesh");
}

// Compute mesh binormals (aka bitangent)
void MeshBinormals(Mesh *mesh)
{
    for (int i = 0; i < mesh->vertexCount; i++)
    {
        Vector3 normal = { mesh->normals[i*3 + 0], mesh->normals[i*3 + 1], mesh->normals[i*3 + 2] };
        Vector3 tangent = { mesh->tangents[i*4 + 0], mesh->tangents[i*4 + 1], mesh->tangents[i*4 + 2] };
        float tangentW = mesh->tangents[i*4 + 3];

        // TODO: Register computed binormal in mesh->binormal?
        // Vector3 binormal = Vector3Multiply(Vector3CrossProduct(normal, tangent), tangentW);
    }
}

//----------------------------------------------------------------------------------
// Module specific Functions Definition
//----------------------------------------------------------------------------------

#if defined(SUPPORT_FILEFORMAT_OBJ)
// Load OBJ mesh data
static Model LoadOBJ(const char *fileName)
{
    Model model = { 0 };

    tinyobj_attrib_t attrib;
    tinyobj_shape_t *meshes = NULL;
    unsigned int meshCount = 0;

    tinyobj_material_t *materials = NULL;
    unsigned int materialCount = 0;

    int dataLength = 0;
    char *data = NULL;

    // Load model data
    FILE *objFile = fopen(fileName, "rb");

    if (objFile != NULL)
    {
        fseek(objFile, 0, SEEK_END);
        long length = ftell(objFile);   // Get file size
        fseek(objFile, 0, SEEK_SET);    // Reset file pointer

        data = (char *)malloc(length);

        fread(data, length, 1, objFile);
        dataLength = length;
        fclose(objFile);
    }

    if (data != NULL)
    {
        unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
        int ret = tinyobj_parse_obj(&attrib, &meshes, &meshCount, &materials, &materialCount, data, dataLength, flags);

        if (ret != TINYOBJ_SUCCESS) TraceLog(LOG_WARNING, "[%s] Model data could not be loaded", fileName);
        else TraceLog(LOG_INFO, "[%s] Model data loaded successfully: %i meshes / %i materials", fileName, meshCount, materialCount);

        // Init model meshes array
        model.meshCount = meshCount;
        model.meshes = (Mesh *)malloc(model.meshCount*sizeof(Mesh));

        // Init model materials array
        model.materialCount = materialCount;
        model.materials = (Material *)malloc(model.materialCount*sizeof(Material));
        model.meshMaterial = (int *)calloc(model.meshCount, sizeof(int));

        /*
        // Multiple meshes data reference
        // NOTE: They are provided as a faces offset
        typedef struct {
            char *name;         // group name or object name
            unsigned int face_offset;
            unsigned int length;
        } tinyobj_shape_t;
        */

        // Init model meshes
        for (int m = 0; m < 1; m++)
        {
            Mesh mesh = { 0 };
            memset(&mesh, 0, sizeof(Mesh));
            mesh.vertexCount = attrib.num_faces*3;
            mesh.triangleCount = attrib.num_faces;
            mesh.vertices = (float *)malloc(mesh.vertexCount*3*sizeof(float));
            mesh.texcoords = (float *)malloc(mesh.vertexCount*2*sizeof(float));
            mesh.normals = (float *)malloc(mesh.vertexCount*3*sizeof(float));

            int vCount = 0;
            int vtCount = 0;
            int vnCount = 0;

            for (int f = 0; f < attrib.num_faces; f++)
            {
                // Get indices for the face
                tinyobj_vertex_index_t idx0 = attrib.faces[3*f + 0];
                tinyobj_vertex_index_t idx1 = attrib.faces[3*f + 1];
                tinyobj_vertex_index_t idx2 = attrib.faces[3*f + 2];

                // TraceLog(LOG_DEBUG, "Face %i index: v %i/%i/%i . vt %i/%i/%i . vn %i/%i/%i\n", f, idx0.v_idx, idx1.v_idx, idx2.v_idx, idx0.vt_idx, idx1.vt_idx, idx2.vt_idx, idx0.vn_idx, idx1.vn_idx, idx2.vn_idx);

                // Fill vertices buffer (float) using vertex index of the face
                for (int v = 0; v < 3; v++) { mesh.vertices[vCount + v] = attrib.vertices[idx0.v_idx*3 + v]; } vCount +=3;
                for (int v = 0; v < 3; v++) { mesh.vertices[vCount + v] = attrib.vertices[idx1.v_idx*3 + v]; } vCount +=3;
                for (int v = 0; v < 3; v++) { mesh.vertices[vCount + v] = attrib.vertices[idx2.v_idx*3 + v]; } vCount +=3;

                // Fill texcoords buffer (float) using vertex index of the face
                // NOTE: Y-coordinate must be flipped upside-down
                mesh.texcoords[vtCount + 0] = attrib.texcoords[idx0.vt_idx*2 + 0];
                mesh.texcoords[vtCount + 1] = 1.0f - attrib.texcoords[idx0.vt_idx*2 + 1]; vtCount += 2;
                mesh.texcoords[vtCount + 0] = attrib.texcoords[idx1.vt_idx*2 + 0];
                mesh.texcoords[vtCount + 1] = 1.0f - attrib.texcoords[idx1.vt_idx*2 + 1]; vtCount += 2;
                mesh.texcoords[vtCount + 0] = attrib.texcoords[idx2.vt_idx*2 + 0];
                mesh.texcoords[vtCount + 1] = 1.0f - attrib.texcoords[idx2.vt_idx*2 + 1]; vtCount += 2;

                // Fill normals buffer (float) using vertex index of the face
                for (int v = 0; v < 3; v++) { mesh.normals[vnCount + v] = attrib.normals[idx0.vn_idx*3 + v]; } vnCount +=3;
                for (int v = 0; v < 3; v++) { mesh.normals[vnCount + v] = attrib.normals[idx1.vn_idx*3 + v]; } vnCount +=3;
                for (int v = 0; v < 3; v++) { mesh.normals[vnCount + v] = attrib.normals[idx2.vn_idx*3 + v]; } vnCount +=3;
            }

            model.meshes[m] = mesh;                 // Assign mesh data to model

            // Assign mesh material for current mesh
            model.meshMaterial[m] = attrib.material_ids[m];
        }

        // Init model materials
        for (int m = 0; m < materialCount; m++)
        {
            // Init material to default
            // NOTE: Uses default shader, only MAP_DIFFUSE supported
            model.materials[m] = LoadMaterialDefault();

            /*
            typedef struct {
                char *name;

                float ambient[3];
                float diffuse[3];
                float specular[3];
                float transmittance[3];
                float emission[3];
                float shininess;
                float ior;          // index of refraction
                float dissolve;     // 1 == opaque; 0 == fully transparent
                // illumination model (see http://www.fileformat.info/format/material/)
                int illum;

                int pad0;

                char *ambient_texname;            // map_Ka
                char *diffuse_texname;            // map_Kd
                char *specular_texname;           // map_Ks
                char *specular_highlight_texname; // map_Ns
                char *bump_texname;               // map_bump, bump
                char *displacement_texname;       // disp
                char *alpha_texname;              // map_d
            } tinyobj_material_t;
            */

            model.materials[m].maps[MAP_DIFFUSE].texture = LoadTexture(materials[m].diffuse_texname);  //char *diffuse_texname; // map_Kd
            model.materials[m].maps[MAP_DIFFUSE].color = (Color){ (float)(materials[m].diffuse[0]*255.0f), (float)(materials[m].diffuse[1]*255.0f), (float)(materials[m].diffuse[2]*255.0f), 255 }; //float diffuse[3];
            model.materials[m].maps[MAP_DIFFUSE].value = 0.0f;

            model.materials[m].maps[MAP_SPECULAR].texture = LoadTexture(materials[m].specular_texname);  //char *specular_texname; // map_Ks
            model.materials[m].maps[MAP_SPECULAR].color = (Color){ (float)(materials[m].specular[0]*255.0f), (float)(materials[m].specular[1]*255.0f), (float)(materials[m].specular[2]*255.0f), 255 }; //float specular[3];
            model.materials[m].maps[MAP_SPECULAR].value = 0.0f;

            model.materials[m].maps[MAP_NORMAL].texture = LoadTexture(materials[m].bump_texname);  //char *bump_texname; // map_bump, bump
            model.materials[m].maps[MAP_NORMAL].color = WHITE;
            model.materials[m].maps[MAP_NORMAL].value = materials[m].shininess;

            model.materials[m].maps[MAP_EMISSION].color = (Color){ (float)(materials[m].emission[0]*255.0f), (float)(materials[m].emission[1]*255.0f), (float)(materials[m].emission[2]*255.0f), 255 }; //float emission[3];

            model.materials[m].maps[MAP_HEIGHT].texture = LoadTexture(materials[m].displacement_texname);  //char *displacement_texname; // disp
        }

        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(meshes, meshCount);
        tinyobj_materials_free(materials, materialCount);
    }

    // NOTE: At this point we have all model data loaded
    TraceLog(LOG_INFO, "[%s] Model loaded successfully in RAM (CPU)", fileName);

    return model;
}
#endif

#if defined(SUPPORT_FILEFORMAT_IQM)
// Load IQM mesh data
static Model LoadIQM(const char *fileName)
{
    #define IQM_MAGIC       "INTERQUAKEMODEL"   // IQM file magic number
    #define IQM_VERSION     2                   // only IQM version 2 supported

    #define BONE_NAME_LENGTH    32          // BoneInfo name string length
    #define MESH_NAME_LENGTH    32          // Mesh name string length

    // IQM file structs
    //-----------------------------------------------------------------------------------
    typedef struct IQMHeader {
        char magic[16];
        unsigned int version;
        unsigned int filesize;
        unsigned int flags;
        unsigned int num_text, ofs_text;
        unsigned int num_meshes, ofs_meshes;
        unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
        unsigned int num_triangles, ofs_triangles, ofs_adjacency;
        unsigned int num_joints, ofs_joints;
        unsigned int num_poses, ofs_poses;
        unsigned int num_anims, ofs_anims;
        unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
        unsigned int num_comment, ofs_comment;
        unsigned int num_extensions, ofs_extensions;
    } IQMHeader;

    typedef struct IQMMesh {
        unsigned int name;
        unsigned int material;
        unsigned int first_vertex, num_vertexes;
        unsigned int first_triangle, num_triangles;
    } IQMMesh;

    typedef struct IQMTriangle {
        unsigned int vertex[3];
    } IQMTriangle;

    // NOTE: Adjacency unused by default
    typedef struct IQMAdjacency {
        unsigned int triangle[3];
    } IQMAdjacency;

    typedef struct IQMJoint {
        unsigned int name;
        int parent;
        float translate[3], rotate[4], scale[3];
    } IQMJoint;

    typedef struct IQMPose {
        int parent;
        unsigned int mask;
        float channeloffset[10];
        float channelscale[10];
    } IQMPose;

    typedef struct IQMAnim {
        unsigned int name;
        unsigned int first_frame, num_frames;
        float framerate;
        unsigned int flags;
    } IQMAnim;

    typedef struct IQMVertexArray {
        unsigned int type;
        unsigned int flags;
        unsigned int format;
        unsigned int size;
        unsigned int offset;
    } IQMVertexArray;

    // NOTE: Bounds unused by default
    typedef struct IQMBounds {
        float bbmin[3], bbmax[3];
        float xyradius, radius;
    } IQMBounds;
    //-----------------------------------------------------------------------------------

    // IQM vertex data types
    typedef enum {
        IQM_POSITION     = 0,
        IQM_TEXCOORD     = 1,
        IQM_NORMAL       = 2,
        IQM_TANGENT      = 3,       // NOTE: Tangents unused by default
        IQM_BLENDINDEXES = 4,
        IQM_BLENDWEIGHTS = 5,
        IQM_COLOR        = 6,       // NOTE: Vertex colors unused by default
        IQM_CUSTOM       = 0x10     // NOTE: Custom vertex values unused by default
    } IQMVertexType;

    Model model = { 0 };

    FILE *iqmFile;
    IQMHeader iqm;

    IQMMesh *imesh;
    IQMTriangle *tri;
    IQMVertexArray *va;
    IQMJoint *ijoint;

    float *vertex = NULL;
    float *normal = NULL;
    float *text = NULL;
    char *blendi = NULL;
    unsigned char *blendw = NULL;

    iqmFile = fopen(fileName, "rb");

    if (iqmFile == NULL)
    {
        TraceLog(LOG_WARNING, "[%s] IQM file could not be opened", fileName);
        return model;
    }

    fread(&iqm,sizeof(IQMHeader), 1, iqmFile);  // Read IQM header

    if (strncmp(iqm.magic, IQM_MAGIC, sizeof(IQM_MAGIC)))
    {
        TraceLog(LOG_WARNING, "[%s] IQM file does not seem to be valid", fileName);
        fclose(iqmFile);
        return model;
    }

    if (iqm.version != IQM_VERSION)
    {
        TraceLog(LOG_WARNING, "[%s] IQM file version is not supported (%i).", fileName, iqm.version);
        fclose(iqmFile);
        return model;
    }

    // Meshes data processing
    imesh = malloc(sizeof(IQMMesh)*iqm.num_meshes);
    fseek(iqmFile, iqm.ofs_meshes, SEEK_SET);
    fread(imesh, sizeof(IQMMesh)*iqm.num_meshes, 1, iqmFile);

    model.meshCount = iqm.num_meshes;
    model.meshes = malloc(sizeof(Mesh)*iqm.num_meshes);

    char name[MESH_NAME_LENGTH];

    for (int i = 0; i < iqm.num_meshes; i++)
    {
        fseek(iqmFile,iqm.ofs_text+imesh[i].name,SEEK_SET);
        fread(name, sizeof(char)*MESH_NAME_LENGTH, 1, iqmFile);     // Mesh name not used...
        model.meshes[i].vertexCount = imesh[i].num_vertexes;

        model.meshes[i].vertices = malloc(sizeof(float)*imesh[i].num_vertexes*3);       // Default vertex positions
        model.meshes[i].normals = malloc(sizeof(float)*imesh[i].num_vertexes*3);        // Default vertex normals
        model.meshes[i].texcoords = malloc(sizeof(float)*imesh[i].num_vertexes*2);      // Default vertex texcoords

        model.meshes[i].boneIds = malloc(sizeof(int)*imesh[i].num_vertexes*4);          // Up-to 4 bones supported!
        model.meshes[i].boneWeights = malloc(sizeof(float)*imesh[i].num_vertexes*4);    // Up-to 4 bones supported!

        model.meshes[i].triangleCount = imesh[i].num_triangles;
        model.meshes[i].indices = malloc(sizeof(unsigned short)*imesh[i].num_triangles*3);

        // Animated verted data, what we actually process for rendering
        // NOTE: Animated vertex should be re-uploaded to GPU (if not using GPU skinning)
        model.meshes[i].animVertices = malloc(sizeof(float)*imesh[i].num_vertexes*3);
        model.meshes[i].animNormals = malloc(sizeof(float)*imesh[i].num_vertexes*3);
    }

    // Triangles data processing
    tri = malloc(sizeof(IQMTriangle)*iqm.num_triangles);
    fseek(iqmFile, iqm.ofs_triangles, SEEK_SET);
    fread(tri, sizeof(IQMTriangle)*iqm.num_triangles, 1, iqmFile);

    for (int m = 0; m < iqm.num_meshes; m++)
    {
        int tcounter = 0;

        for (int i = imesh[m].first_triangle; i < imesh[m].first_triangle+imesh[m].num_triangles; i++)
        {
            // IQM triangles are stored counter clockwise, but raylib sets opengl to clockwise drawing, so we swap them around
            model.meshes[m].indices[tcounter+2] = tri[i].vertex[0] - imesh[m].first_vertex;
            model.meshes[m].indices[tcounter+1] = tri[i].vertex[1] - imesh[m].first_vertex;
            model.meshes[m].indices[tcounter] = tri[i].vertex[2] - imesh[m].first_vertex;
            tcounter += 3;
        }
    }

    // Vertex arrays data processing
    va = malloc(sizeof(IQMVertexArray)*iqm.num_vertexarrays);
    fseek(iqmFile, iqm.ofs_vertexarrays, SEEK_SET);
    fread(va, sizeof(IQMVertexArray)*iqm.num_vertexarrays, 1, iqmFile);

    for (int i = 0; i < iqm.num_vertexarrays; i++)
    {
        switch (va[i].type)
        {
            case IQM_POSITION:
            {
                vertex = malloc(sizeof(float)*iqm.num_vertexes*3);
                fseek(iqmFile, va[i].offset, SEEK_SET);
                fread(vertex, sizeof(float)*iqm.num_vertexes*3, 1, iqmFile);

                for (int m = 0; m < iqm.num_meshes; m++)
                {
                    int vCounter = 0;
                    for (int i = imesh[m].first_vertex*3; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*3; i++)
                    {
                        model.meshes[m].vertices[vCounter] = vertex[i];
                        model.meshes[m].animVertices[vCounter] = vertex[i];
                        vCounter++;
                    }
                }
            } break;
            case IQM_NORMAL:
            {
                normal = malloc(sizeof(float)*iqm.num_vertexes*3);
                fseek(iqmFile, va[i].offset, SEEK_SET);
                fread(normal, sizeof(float)*iqm.num_vertexes*3, 1, iqmFile);

                for (int m = 0; m < iqm.num_meshes; m++)
                {
                    int vCounter = 0;
                    for (int i = imesh[m].first_vertex*3; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*3; i++)
                    {
                        model.meshes[m].normals[vCounter] = normal[i];
                        model.meshes[m].animNormals[vCounter] = normal[i];
                        vCounter++;
                    }
                }
            } break;
            case IQM_TEXCOORD:
            {
                text = malloc(sizeof(float)*iqm.num_vertexes*2);
                fseek(iqmFile, va[i].offset, SEEK_SET);
                fread(text, sizeof(float)*iqm.num_vertexes*2, 1, iqmFile);

                for (int m = 0; m < iqm.num_meshes; m++)
                {
                    int vCounter = 0;
                    for (int i = imesh[m].first_vertex*2; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*2; i++)
                    {
                        model.meshes[m].texcoords[vCounter] = text[i];
                        vCounter++;
                    }
                }
            } break;
            case IQM_BLENDINDEXES:
            {
                blendi = malloc(sizeof(char)*iqm.num_vertexes*4);
                fseek(iqmFile, va[i].offset, SEEK_SET);
                fread(blendi, sizeof(char)*iqm.num_vertexes*4, 1, iqmFile);

                for (int m = 0; m < iqm.num_meshes; m++)
                {
                    int boneCounter = 0;
                    for (int i = imesh[m].first_vertex*4; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*4; i++)
                    {
                        model.meshes[m].boneIds[boneCounter] = blendi[i];
                        boneCounter++;
                    }
                }
            } break;
            case IQM_BLENDWEIGHTS:
            {
                blendw = malloc(sizeof(unsigned char)*iqm.num_vertexes*4);
                fseek(iqmFile,va[i].offset,SEEK_SET);
                fread(blendw,sizeof(unsigned char)*iqm.num_vertexes*4,1,iqmFile);

                for (int m = 0; m < iqm.num_meshes; m++)
                {
                    int boneCounter = 0;
                    for (int i = imesh[m].first_vertex*4; i < (imesh[m].first_vertex + imesh[m].num_vertexes)*4; i++)
                    {
                        model.meshes[m].boneWeights[boneCounter] = blendw[i]/255.0f;
                        boneCounter++;
                    }
                }
            } break;
        }
    }

    // Bones (joints) data processing
    ijoint = malloc(sizeof(IQMJoint)*iqm.num_joints);
    fseek(iqmFile, iqm.ofs_joints, SEEK_SET);
    fread(ijoint, sizeof(IQMJoint)*iqm.num_joints, 1, iqmFile);

    model.boneCount = iqm.num_joints;
    model.bones = malloc(sizeof(BoneInfo)*iqm.num_joints);
    model.bindPose = malloc(sizeof(Transform)*iqm.num_joints);

    for (int i = 0; i < iqm.num_joints; i++)
    {
        // Bones
        model.bones[i].parent = ijoint[i].parent;
        fseek(iqmFile, iqm.ofs_text + ijoint[i].name, SEEK_SET);
        fread(model.bones[i].name,sizeof(char)*BONE_NAME_LENGTH, 1, iqmFile);

        // Bind pose (base pose)
        model.bindPose[i].translation.x = ijoint[i].translate[0];
        model.bindPose[i].translation.y = ijoint[i].translate[1];
        model.bindPose[i].translation.z = ijoint[i].translate[2];

        model.bindPose[i].rotation.x = ijoint[i].rotate[0];
        model.bindPose[i].rotation.y = ijoint[i].rotate[1];
        model.bindPose[i].rotation.z = ijoint[i].rotate[2];
        model.bindPose[i].rotation.w = ijoint[i].rotate[3];

        model.bindPose[i].scale.x = ijoint[i].scale[0];
        model.bindPose[i].scale.y = ijoint[i].scale[1];
        model.bindPose[i].scale.z = ijoint[i].scale[2];
    }

    // Build bind pose from parent joints
    for (int i = 0; i < model.boneCount; i++)
    {
        if (model.bones[i].parent >= 0)
        {
            model.bindPose[i].rotation = QuaternionMultiply(model.bindPose[model.bones[i].parent].rotation, model.bindPose[i].rotation);
            model.bindPose[i].translation = Vector3RotateByQuaternion(model.bindPose[i].translation, model.bindPose[model.bones[i].parent].rotation);
            model.bindPose[i].translation = Vector3Add(model.bindPose[i].translation, model.bindPose[model.bones[i].parent].translation);
            model.bindPose[i].scale = Vector3MultiplyV(model.bindPose[i].scale, model.bindPose[model.bones[i].parent].scale);
        }
    }

    fclose(iqmFile);
    free(imesh);
    free(tri);
    free(va);
    free(vertex);
    free(normal);
    free(text);
    free(blendi);
    free(blendw);
    free(ijoint);

    return model;
}
#endif

#if defined(SUPPORT_FILEFORMAT_GLTF)
// Load glTF mesh data
static Model LoadGLTF(const char *fileName)
{
    Model model = { 0 };

    // glTF file loading
    FILE *gltfFile = fopen(fileName, "rb");

    if (gltfFile == NULL)
    {
        TraceLog(LOG_WARNING, "[%s] glTF file could not be opened", fileName);
        return model;
    }

    fseek(gltfFile, 0, SEEK_END);
    int size = ftell(gltfFile);
    fseek(gltfFile, 0, SEEK_SET);

    void *buffer = malloc(size);
    fread(buffer, size, 1, gltfFile);

    fclose(gltfFile);

    // glTF data loading
    cgltf_options options = {0};
    cgltf_data *data;
    cgltf_result result = cgltf_parse(&options, buffer, size, &data);

    free(buffer);

    if (result == cgltf_result_success)
    {
        // printf("Type: %u\n", data.file_type);
        // printf("Version: %d\n", data.version);
        // printf("Meshes: %lu\n", data.meshes_count);

        // TODO: Process glTF data and map to model

        // NOTE: data.buffers[] should be loaded to model.meshes and data.images[] should be loaded to model.materials
        // Use buffers[n].uri and images[n].uri... or use cgltf_load_buffers(&options, data, fileName);

        cgltf_free(data);
    }
    else TraceLog(LOG_WARNING, "[%s] glTF data could not be loaded", fileName);

    return model;
}
#endif
