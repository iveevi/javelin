#pragma once

#include "../thunder/tracked_buffer.hpp"
#include "../thunder/linkage_unit.hpp"
#include "logging.hpp"
#include "io.hpp"

namespace jvl::ire {

namespace detail {

extern std::filesystem::path trace_destination;

} // namespace detail

void set_trace_destination(const std::filesystem::path &);

template <typename ... Args>
void trace_unit(const std::string &name, const jvl::Stage &stage, Args &&...args)
{
	MODULE(trace_unit);

	std::vector <jvl::thunder::TrackedBuffer> tracked { args... };

	jvl::thunder::LinkageUnit unit;
	for (auto &tb : tracked)
		unit.add(tb);

	// TODO: stage_extension method
	std::string ext;
	switch (stage) {
	case jvl::Stage::vertex:
		ext = "vert";
		break;
	case jvl::Stage::fragment:
		ext = "frag";
		break;
	case jvl::Stage::compute:
		ext = "comp";
		break;
	case jvl::Stage::mesh:
		ext = "mesh";
		break;
	case jvl::Stage::task:
		ext = "task";
		break;
	case jvl::Stage::ray_generation:
		ext = "rgen";
		break;
	case jvl::Stage::closest_hit:
		ext = "rchit";
		break;
	case jvl::Stage::miss:
		ext = "rmiss";
		break;
	default:
		JVL_ABORT("unsupported stage #{}", (uint32_t) stage);
	}

	std::filesystem::create_directories(detail::trace_destination);

	std::string dst = detail::trace_destination / (name + "." + ext);
	for (auto &ftn : unit.functions) {
		ftn.write_assembly(dst + "." + ftn.name + ".jvl");
		ftn.graphviz(dst + "." + ftn.name + ".dot");
	}

	jvl::io::write_lines(dst, unit.generate_glsl());
}

} // namesapce jvl::ire