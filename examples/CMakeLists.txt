add_custom_target(examples)

include_directories(.)

add_subdirectory(common)

set(EXAMPLES
	colored
	compute
	features
	font
	meshlet
	ngf
	normals
	palette
	particles
	pathtracing
	raytracing
	# shadertoy
)

foreach(ex ${EXAMPLES})
	add_subdirectory(${ex})
	add_dependencies(examples ${ex})
endforeach()