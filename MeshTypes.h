#ifndef MESHTYPES_H
#define MESHTYPES_H

// Useful stuff for working with meshes

// Mesh part type identifiers

#define MESHPART_TYPE_MESH		1
#define MESHPART_TYPE_NULL		2
#define MESHPART_TYPE_SKELETAL	3
#define MESHPART_TYPE_CAMERA	4
#define MESHPART_TYPE_EMITTER	5
#define MESHPART_TYPE_REFERENCE 6

// Texture types
// When adding new ones, don't forget to add a render setup routine 
// in texture.cpp.

#define TEXTURE_TYPES				6

#define TEXTURE_TYPE_NONE			-1
#define TEXTURE_TYPE_DIFFUSE		0
#define TEXTURE_TYPE_BUMPMAP		1
#define TEXTURE_TYPE_ENVMAP			2
#define TEXTURE_TYPE_SCREENSPACE	3
#define TEXTURE_TYPE_DIFFUSE2		4
#define TEXTURE_TYPE_SNOWLAYER		5

#endif