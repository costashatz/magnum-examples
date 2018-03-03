layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

/* inputs from vertex shader; stored in arrays */
#ifdef TEXTURED
in vec2 textureCoords[];
#endif
in vec3 transformedNormal[];

/* uniforms */
layout(location = 1)
uniform mat4 projectionMatrixX;

layout(location = 2)
uniform mat4 projectionMatrixY;

layout(location = 3)
uniform mat4 projectionMatrixZ;

layout(location = 4)
uniform int voxelDimensions;

layout(location = 5)
uniform float voxelSize;

layout(location = 6)
uniform float voxelWorldSize;

/* Conservative rasterization is on by default */
layout(location = 9)
uniform int conservativeRasterization = 1;

/* outputs to fragment shader */
#ifdef TEXTURED
out vec2 texCoords;
#endif
out vec3 worldPosition;
out vec3 worldNormal;
out flat int dominantAxis;
out flat int voxelDims;
out vec4 boundingBox;
out vec3 texPos;

void main() {
    /* pass through voxel dimensions */
    voxelDims = voxelDimensions;
    /* get normal from triangle */
    vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 faceNormal = normalize(cross(p1, p2));

    float nDotX = abs(faceNormal.x);
    float nDotY = abs(faceNormal.y);
    float nDotZ = abs(faceNormal.z);

    /* 1 = x axis dominant, 2 = y axis dominant, 3 = z axis dominant */
    dominantAxis = (nDotX > nDotY && nDotX > nDotZ) ? 1 : (nDotY > nDotX && nDotY > nDotZ) ? 2 : 3;
    mat4 projectionMatrix = dominantAxis == 1 ? projectionMatrixX : dominantAxis == 2 ? projectionMatrixY : projectionMatrixZ;

    float scale = 1. / voxelWorldSize;

    vec4 vsProjs[3] = vec4[3] (projectionMatrix * gl_in[0].gl_Position,
                                projectionMatrix * gl_in[1].gl_Position,
                                projectionMatrix * gl_in[2].gl_Position);

    /* Pass axis-aligned, extended bounding box in NDCs */
    boundingBox = vec4(min(vsProjs[0].xy, min(vsProjs[1].xy, vsProjs[2].xy)),
                                max(vsProjs[0].xy, max(vsProjs[1].xy, vsProjs[2].xy)));
    boundingBox = (boundingBox * vec4(0.5) + vec4(0.5)) * voxelDimensions;

    /* for code visibility */
    mat4 viewProjectionInverseMatrix = mat4(1.);

    /* Conservative rasterization */
    if(conservativeRasterization > 0) {
        viewProjectionInverseMatrix = inverse(projectionMatrix);
        vec4 projNormal = transpose(viewProjectionInverseMatrix) * vec4(faceNormal, 0);

        boundingBox.xy -= vec2(1.0);
        boundingBox.zw += vec2(1.0);

        /* Calculate normal equation of triangle plane. */
        vec3 normal = cross(vsProjs[1].xyz - vsProjs[0].xyz, vsProjs[0].xyz - vsProjs[2].xyz);
        normal = projNormal.xyz;
        float d = dot(vsProjs[0].xyz, normal);
        float normalSign = (projNormal.z > 0) ? 1.0 : -1.0;

        /* Convert edges to homogeneous line equations and dilate triangle.
           See:  http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter42.html
         */
        vec3 planes[3]; vec3 intersection[3]; float z[3];
        vec2 halfPixelSize = vec2(1.0 / voxelDimensions);
        planes[0] = cross(vsProjs[0].xyw - vsProjs[2].xyw, vsProjs[2].xyw);
        planes[1] = cross(vsProjs[1].xyw - vsProjs[0].xyw, vsProjs[0].xyw);
        planes[2] = cross(vsProjs[2].xyw - vsProjs[1].xyw, vsProjs[1].xyw);
        planes[0].z += normalSign * dot(halfPixelSize, abs(planes[0].xy));
        planes[1].z += normalSign * dot(halfPixelSize, abs(planes[1].xy));
        planes[2].z += normalSign * dot(halfPixelSize, abs(planes[2].xy));
        intersection[0] = cross(planes[0], planes[1]);
        intersection[1] = cross(planes[1], planes[2]);
        intersection[2] = cross(planes[2], planes[0]);
        intersection[0] /= intersection[0].z;
        intersection[1] /= intersection[1].z;
        intersection[2] /= intersection[2].z;
        z[0] = (-intersection[0].x * normal.x - intersection[0].y * normal.y + d) / normal.z;
        z[1] = (-intersection[1].x * normal.x - intersection[1].y * normal.y + d) / normal.z;
        z[2] = (-intersection[2].x * normal.x - intersection[2].y * normal.y + d) / normal.z;
        vsProjs[0].xyz = vec3(intersection[0].xy, z[0]);
        vsProjs[1].xyz = vec3(intersection[1].xy, z[1]);
        vsProjs[2].xyz = vec3(intersection[2].xy, z[2]);
    }

    /* For every vertex sent in vertices */
    for(int i = 0; i < gl_in.length(); i++) {
#ifdef TEXTURED
        texCoords = textureCoords[i];
#endif
        worldNormal = transformedNormal[i];
        worldPosition = gl_in[i].gl_Position.xyz / gl_in[i].gl_Position.w;
        gl_Position = vsProjs[i];

        if(conservativeRasterization > 0) {
            texPos = (viewProjectionInverseMatrix * vsProjs[i]).xyz * scale + 0.5;
        }
        else {
            texPos = gl_in[i].gl_Position.xyz * scale + 0.5;
        }
        EmitVertex();
    }

    /* Finished */
    EndPrimitive();
}