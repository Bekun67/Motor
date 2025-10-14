#include "Assimp/include/LoadFBX.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"

// Stream log messages to Debug window
struct
	aiLogStream stream;
stream = aiGetPredefinedLogStream(aiDefaultLogStream_DEBUGGER, nullptr);
aiAttachLogStream(&stream);

void CleanUp() {
	aiDetachAllLogStreams();
}
