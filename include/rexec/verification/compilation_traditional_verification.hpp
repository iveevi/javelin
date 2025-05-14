#pragma once

#include "../../ire/mirror/solid.hpp"
#include "../class.hpp"
#include "../context/fragment.hpp"
#include "../context/vertex.hpp"
#include "../pipeline/indicators.hpp"
#include "compilation_results.hpp"
#include "compilation_status.hpp"

namespace jvl::rexec {

template <resource_collection_class Resources>
constexpr auto synthesize_vertex_element()
{
	if constexpr (Resources::size) {

	}
}

template <resource_layout_in Fin, resource_layout_out Vout, resource_layout_out ... Vrest>
constexpr auto verify_vertex_to_fragment_layouts() -> PipelineCompilationStatus
{
	if constexpr (Fin::location == Vout::location) {
		using Ftype = typename Fin::value_type;
		using Vtype = typename Vout::value_type;

		if constexpr (std::same_as <Ftype, Vtype>)
			return PipelineCompilationStatus::eSuccess;
		else
			return PipelineCompilationStatus::eIncompatibleLayout;
	}

	if constexpr (sizeof...(Vrest))
		return verify_vertex_to_fragment_layouts <Fin, Vrest...> ();
	
	return PipelineCompilationStatus::eMissingLayout;
}

template <resource_layout_in ... Fins,
	 resource_layout_out ... Vouts>
constexpr auto verify_vertex_to_fragment_layouts(const resource_collection <Vouts...> &,
						 const resource_collection <Fins...> &)
						 -> PipelineCompilationStatus
{
	return (verify_vertex_to_fragment_layouts <Fins, Vouts> () || ...);
}

template <typename VertexConstants, typename FragmentConstants>
struct VerifyTraditionalPushContantsResult {
	PipelineCompilationStatus status;

	using vconst = VertexConstants;
	using fconst = FragmentConstants;
};

template <resource_collection_class Vpcs, resource_collection_class Fpcs>
constexpr auto verify_vertex_to_fragment_push_constants()
{
	using Fail = VerifyTraditionalPushContantsResult <void, void>;

	static_assert(Vpcs::size <= 1);
	static_assert(Fpcs::size <= 1);

	if constexpr (Vpcs::size && Fpcs::size) {
		using vpc = Vpcs::head;
		using fpc = Fpcs::head;

		using vpc_solid = ire::solid_t <typename vpc::value_type>;
		using fpc_solid = ire::solid_t <typename fpc::value_type>;

		// TODO: alignment? alignup(X, 4)?
		constexpr size_t vbegin = vpc::offset;
		constexpr size_t vend = vpc::offset + sizeof(vpc_solid);
		
		constexpr size_t fbegin = fpc::offset;
		constexpr size_t fend = fpc::offset + sizeof(fpc_solid);

		constexpr bool overlapping = std::max(vbegin, fbegin) < std::min(vend, fend);

		if constexpr (overlapping) {
			return Fail(PipelineCompilationStatus::eOverlappingPushConstants);
		} else {
			using vconst = PushConstantIndicator <vpc_solid, vbegin>;
			using fconst = PushConstantIndicator <fpc_solid, fbegin>;

			return VerifyTraditionalPushContantsResult <vconst, fconst>
				(PipelineCompilationStatus::eSuccess);
		}
	} else {
		return Fail(PipelineCompilationStatus::eSuccess);
	}
}

template <vertex_class Vertex, fragment_class Fragment>
constexpr auto verify_traditional()
{
	// Synthesize vertex element
	using vertex_layout_ins = Vertex::layout_ins;

	using vertex = decltype(synthesize_vertex_element <vertex_layout_ins> ());

	static_assert(std::same_as <vertex, void>);

	// Verify layout inputs
	using vertex_layout_outs = Vertex::layout_outs;
	using fragment_layout_ins = Fragment::layout_ins;

	constexpr auto layout_result = verify_vertex_to_fragment_layouts(
		vertex_layout_outs(),
		fragment_layout_ins());
	
	// Verify push constants
	using vertex_push_constants = Vertex::push_constants;
	using fragment_push_constants = Fragment::push_constants;

	constexpr auto constants_result = verify_vertex_to_fragment_push_constants
		<vertex_push_constants, fragment_push_constants>();
	
	using VTPCR = decltype(constants_result);
	using vconst = VTPCR::vconst;
	using fconst = VTPCR::fconst;

	// Verify bindable resources
	// TODO: check bindable resources...

	constexpr auto status = layout_result + constants_result.status;

	return VerifyTraditionalResult <vertex, vconst, fconst> (status);
}

} // namespace jvl::rexec