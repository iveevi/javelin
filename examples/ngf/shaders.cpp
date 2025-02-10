#include "shaders.hpp"

Procedure <void> task = procedure <void> ("main") << []()
{
	push_constant <ViewInfo> pc;

	task_payload <Payload> payload;

	payload.pindex = gl_GlobalInvocationID.x;
	payload.resolution = pc.resolution;

	u32 groups = (payload.resolution - 1 + 6) / 7;

	EmitMeshTasksEXT(groups, groups, 1);
};