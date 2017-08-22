#version 330

layout (triangles_adjacency) in;    // six vertices in
layout (triangle_strip, max_vertices = 18) out; // 4 per quad * 3 triangle vertices + 6 for near/far caps

in vec3 PosL[]; // an array of 6 vertices (triangle with adjacency)

uniform vec3 gLightPos;
//uniform mat4 gWVP;
uniform mat4 gVP;
uniform mat4 gWorld;

float EPSILON = 0.0001;

// Emit a quad using a triangle strip
void EmitQuad(vec3 StartVertex, vec3 EndVertex)
{
    // Vertex #1: the starting vertex (just a tiny bit below the original edge)
    vec3 LightDir = normalize(StartVertex - gLightPos);   
    gl_Position = gVP * vec4((StartVertex + LightDir * EPSILON), 1.0);
    EmitVertex();
 
    // Vertex #2: the starting vertex projected to infinity
    gl_Position = gVP * vec4(LightDir, 0.0);
    EmitVertex();
    
    // Vertex #3: the ending vertex (just a tiny bit below the original edge)
    LightDir = normalize(EndVertex - gLightPos);
    gl_Position = gVP * vec4((EndVertex + LightDir * EPSILON), 1.0);
    EmitVertex();
    
    // Vertex #4: the ending vertex projected to infinity
    gl_Position = gVP * vec4(LightDir , 0.0);
    EmitVertex();

    EndPrimitive();            
}


void main()
{
	vec3 PosW0 = vec3(gWorld * vec4(PosL[0], 1));
	vec3 PosW1 = vec3(gWorld * vec4(PosL[1], 1));
	vec3 PosW2 = vec3(gWorld * vec4(PosL[2], 1));
	vec3 PosW3 = vec3(gWorld * vec4(PosL[3], 1));
	vec3 PosW4 = vec3(gWorld * vec4(PosL[4], 1));
	vec3 PosW5 = vec3(gWorld * vec4(PosL[5], 1));

	/*
    vec3 e1 = PosL[2] - PosL[0];
    vec3 e2 = PosL[4] - PosL[0];
    vec3 e3 = PosL[1] - PosL[0];
    vec3 e4 = PosL[3] - PosL[2];
    vec3 e5 = PosL[4] - PosL[2];
    vec3 e6 = PosL[5] - PosL[0];
	*/

    vec3 e1 = PosW2 - PosW0;
    vec3 e2 = PosW4 - PosW0;
    vec3 e3 = PosW1 - PosW0;
    vec3 e4 = PosW3 - PosW2;
    vec3 e5 = PosW4 - PosW2;
    vec3 e6 = PosW5 - PosW0;

    vec3 Normal = normalize(cross(e1,e2));
    vec3 LightDir = normalize(gLightPos - PosW0);

    // Handle only light facing triangles
    if (dot(Normal, LightDir) > 0) {

        Normal = cross(e3,e1);

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosW0;
            vec3 EndVertex = PosW2;
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e4,e5);
        LightDir = gLightPos - PosW2;

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosW2;
            vec3 EndVertex = PosW4;
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e2,e6);
        LightDir = gLightPos - PosW4;

        if (dot(Normal, LightDir) <= 0) {
            vec3 StartVertex = PosW4;
            vec3 EndVertex = PosW0;
            EmitQuad(StartVertex, EndVertex);
        }

        // render the front cap
        LightDir = (normalize(PosW0 - gLightPos));
        gl_Position = gVP * vec4((PosW0 + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir = (normalize(PosW2 - gLightPos));
        gl_Position = gVP * vec4((PosW2 + LightDir * EPSILON), 1.0);
        EmitVertex();

        LightDir = (normalize(PosW4 - gLightPos));
        gl_Position = gVP * vec4((PosW4 + LightDir * EPSILON), 1.0);
        EmitVertex();
        EndPrimitive();
 
        // render the back cap
        LightDir = PosW0 - gLightPos;
        gl_Position = gVP * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir = PosW4 - gLightPos;
        gl_Position = gVP * vec4(LightDir, 0.0);
        EmitVertex();

        LightDir = PosW2 - gLightPos;
        gl_Position = gVP * vec4(LightDir, 0.0);
        EmitVertex();

        EndPrimitive();
    }
}