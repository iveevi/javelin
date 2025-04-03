// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <common/io.hpp>

#include <ire.hpp>
#include <rexec.hpp>

using namespace jvl;
using namespace jvl::ire;
using namespace jvl::rexec;

MODULE(ire);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: get line numbers for each invocation if possible?
// using source location if available

// Type safe options: shaders as functors...

// In a separate module, author a shader module
// which defines an environment of available
// resources and the corresponding methods under that scope...

// Example:
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};

template <size_t Offset>
$rexec_callable(Projection, $push_constant(MVP, Offset))
{
	using self = Projection;

	// Forward declaring to define an interface...
	$rexec_subroutine(vec4, apply, vec3 p) {
		$return $constants.project(p);
	};
};

// Appending a procedure with REXEC information
template <rexec_class R, typename ... Args>
struct rexec_method : Procedure <Args...> {
	using rexec = R;

	rexec_method(const Procedure <Args...> &proc) : Procedure <Args...> (proc) {}
};

template <rexec_class R, typename ... Args>
bool rexec_method_pass(const rexec_method <R, Args...> &);

template <typename T>
concept rexec_class_method = requires(const T &value) {
	{ rexec_method_pass(value) } -> std::same_as <bool>;
};

#define $rexec_entrypoint_ext(name)					\
	static inline rexec_method <self, void> name			\
		= ::jvl::ire::ProcedureBuilder("main")			\
		<< _initializer()					\
		* [_returner = _return_igniter <void> ()]() -> void

template <rexec_class R, generic_or_void Ret, typename F>
struct rexec_manifest_skeleton {
	using proc = void;
};

template <rexec_class R, generic_or_void Ret, typename ... Args>
struct rexec_manifest_skeleton <R, Ret, void (*)(Args...)> {
	using proc = rexec_method <R, Ret, Args...>;
};

#define $rexec_subroutine_ext(R, name, ...)							\
	static inline rexec_manifest_skeleton <self, R, void (*)(__VA_ARGS__)> ::proc name	\
		= ::jvl::ire::ProcedureBuilder <R> (#name)					\
		<< _initializer()								\
		* [_returner = _return_igniter <R> ()](__VA_ARGS__) -> void

$rexec_vertex(VShader,
	$layout_in(vec3, 0),
	$layout_in(vec3, 1),
	$layout_out(vec3, 0),
	$push_constant(MVP, 0)
)
{
	$rexec_subroutine_ext(vec3, value) {
		$return vec3(12, 13, 14);
	};

	$rexec_entrypoint_ext(main) {
		// ...such that the REXEC can still be used appropriately
		gl_Position = $use(Projection <0>).apply($lin(0));
	};
};

$rexec_fragment(FShader,
	$layout_in(vec3, 0),
	$layout_out(vec4, 0)
)
{
	$rexec_entrypoint_ext(main) {
		$lout(0) = vec4(0.5 + 0.5 * $lin(0), 1);
	};
};

// TODO: for partial procedures, need to return
// rexec_procedure when specializing...

// Rectifying methods which belong to certain REXEC contexts...
template <typename T>
concept vertex_class_method = rexec_class_method <T> && vertex_class <typename T::rexec>;

template <typename T>
concept fragment_class_method = rexec_class_method <T> && fragment_class <typename T::rexec>;

// Rectifying entrypoints for each stage class
bool entrypoint_pass(const Procedure <void> &);

template <typename T>
concept entrypoint_class = requires(const T &value) {
	{ entrypoint_pass(value) } -> std::same_as <bool>;
};

template <typename T>
concept vertex_class_entrypoint = vertex_class_method <T> && entrypoint_class <T>;

template <typename T>
concept fragment_class_entrypoint = fragment_class_method <T> && entrypoint_class <T>;

template <generic_or_void R, typename ... Args>
bool procedure_pass(const Procedure <R, Args...> &);

template <typename T>
concept procedure_class = requires(const T &value) {
	{ procedure_pass(value) } -> std::same_as <bool>;
};

// Package for traditional graphics pipeline
// TODO: move to pipelines/traditional.hpp
#include <vulkan/vulkan.hpp>

using BindingInfo = vk::DescriptorSetLayoutBinding;

struct no_push_constant_range {};

template <resource_push_constant ... Constants>
constexpr auto push_constant_range_for_stage(const resource_collection <Constants...> &,
					     const vk::ShaderStageFlags &stage)
{
	static_assert(sizeof...(Constants) == 0, "each stage can only have at most one push constant");
	return no_push_constant_range();
}

template <resource_push_constant T>
constexpr auto push_constant_range_for_stage(const resource_collection <T> &,
					     const vk::ShaderStageFlags &stage)
{
	using value_type = T::value_type;

	return vk::PushConstantRange()
		.setOffset(T::offset)
		.setSize(sizeof(solid_t <value_type>))
		.setStageFlags(stage);
}

template <resource ... Resources>
constexpr auto push_constant_range_for_resources_and_stage(const resource_collection <Resources...> &,
							   const vk::ShaderStageFlags &stage)
{
	using filtered = filter_push_constant <resource_collection <Resources...>> ::group;
	return push_constant_range_for_stage(filtered(), stage);
}

// NOTE: prototype

// TODO: vertices_t <...> for vertex buffers where each element is vertex_t

// TODO: also design a variable PendingDescriptor <...> type...
// an engine should only use PendingDescriptor <> = ValidDescriptor during rendering
// and a PendingPipeline <...> only accepts ValidDescriptors...

// template <typename ... T>
// struct PendingPipeline {
// 	~PendingPipeline() {
		// static_assert(sizeof...(T) == 0);
// 	}
// };

// TODO: sort when filtering layout inputs?
template <resource_layout_in ... Ls>
struct sorted_layout_inputs {};

template <resource_layout_in Element, resource_layout_in ... Elements>
constexpr auto _sorted_layout_inputs_insert(const sorted_layout_inputs <Elements...> &)
{
	return sorted_layout_inputs <Element, Elements...> ();
}

template <resource_layout_in Element, typename List>
using sorted_layout_inputs_insert = decltype(_sorted_layout_inputs_insert <Element> (List()));

template <>
struct sorted_layout_inputs <> {
	template <resource_layout_in Element>
	using add = sorted_layout_inputs <Element>;
};

template <resource_layout_in Head, resource_layout_in ... Tail>
struct sorted_layout_inputs <Head, Tail...> {
	template <resource_layout_in Element>
	static constexpr auto _add() {
		if constexpr (Element::location < Head::location) {
			return sorted_layout_inputs <Element, Head, Tail...> ();
		} else {
			using fixed = sorted_layout_inputs <Tail...> ::template add <Element>;
			using complete = sorted_layout_inputs_insert <Head, fixed>;
			return complete();
		}
	}
	
	template <resource_layout_in Element>
	using add = decltype(_add <Element> ());
};

template <resource_layout_in ... Elements>
constexpr auto sort_layout_inputs(const resource_collection <Elements...> &);

template <resource_layout_in Head, resource_layout_in ... Tail>
constexpr auto sort_layout_inputs(const resource_collection <Head, Tail...> &)
{
	if constexpr (sizeof...(Tail)) {
		using next = decltype(sort_layout_inputs(resource_collection <Tail...> ()));
		return typename next::template add <Head> ();
	} else {
		return sorted_layout_inputs <Head> ();
	}
}

template <>
constexpr auto sort_layout_inputs <> (const resource_collection <> &)
{
	return sorted_layout_inputs <> ();
}

// TODO: vec3 with alterate precision...
template <resource_layout_in Required>
constexpr bool verify_in_from_out_set(const resource_collection <> &)
{
	return false;
}

template <resource_layout_in Required, resource_layout_out Head, resource_layout_out ... Tail>
constexpr bool verify_in_from_out_set(const resource_collection <Head, Tail...> &)
{
	if constexpr (Required::location == Head::location) {
		return std::same_as <typename Required::value_type, typename Head::value_type>;
	} else if constexpr (sizeof...(Tail)) {
		return verify_layout_in_from_out <Required> (resource_collection <Tail...> ());
	} else {
		return false;
	}
}

template <resource_layout_in ... Required, resource_layout_out ... Given>
constexpr bool verify_in_set_from_out_set(const resource_collection <Required...> &,
					  const resource_collection <Given...> &given)
{
	return (verify_in_from_out_set <Required> (given) && ...);
}

template <generic Symbolic>
constexpr vk::Format format_lookup()
{
	// TODO: format is not necessarily same as type... for now its fine
	if constexpr (std::same_as <Symbolic, vec3>) {
		return vk::Format::eR32G32B32Sfloat;
	} else {
		static_assert(false,
			"unsupported vertex layout input, "
			"type is not registered in format lookup");
	}
}

// TODO: format size...
template <resource_layout_in L, resource_layout_in ... Ls>
constexpr auto vertex_input_attributes(size_t offset = 0)
{
	using value_type = typename L::value_type;

	std::array <vk::VertexInputAttributeDescription, 1 + sizeof...(Ls)> result;

	result[0] = vk::VertexInputAttributeDescription()
		.setBinding(0)
		.setFormat(format_lookup <value_type> ())
		.setOffset(offset)
		.setLocation(L::location);
	
	if constexpr (sizeof...(Ls)) {
		size_t size = sizeof(solid_t <value_type>);
		auto next = vertex_input_attributes <Ls...> (offset + size);
		std::copy(next.begin(), next.end(), result.begin() + 1);
	}

	return result;
}

// TODO: use tuple_storage
template <resource_layout_in ... Ls>
struct vertex_storage : std::tuple <solid_t <typename Ls::value_type>...> {
	static constexpr vk::VertexInputBindingDescription binding
		= vk::VertexInputBindingDescription()
			.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(sizeof(vertex_storage));

	static constexpr auto attributes
		= vertex_input_attributes <Ls...> (0);
};

template <resource_layout_in ... Ls>
constexpr auto vertex_storage_from_inputs(const sorted_layout_inputs <Ls...> &)
{
	return vertex_storage <Ls...> ();
}

template <typename T>
concept vertex_buffer_class = true;

template <typename T>
concept push_constant_class = true;

template <vertex_buffer_class VB, push_constant_class PC>
struct TraditionalPipeline {
	using vertices_t = VB;
};

template <typename V, typename F>
struct TraditionalPipelineGroup {
	const V &vertex;
	const F &fragment;

	void compile(const vk::Device &device, const vk::RenderPass renderpass) {
		using vresources = V::rexec::collection;
		using fresources = F::rexec::collection;

		// Configure push constant ranges
		auto vpc = push_constant_range_for_resources_and_stage(vresources(), vk::ShaderStageFlagBits::eVertex);
		auto fpc = push_constant_range_for_resources_and_stage(fresources(), vk::ShaderStageFlagBits::eFragment);

		std::vector <vk::PushConstantRange> ranges;
		if constexpr (std::same_as <decltype(vpc), vk::PushConstantRange>)
			ranges.emplace_back(vpc);
		if constexpr (std::same_as <decltype(fpc), vk::PushConstantRange>)
			ranges.emplace_back(fpc);

		// Configure descriptor layout
		auto dsl_info = vk::DescriptorSetLayoutCreateInfo();

		// TODO: finish...

		auto dsl = device.createDescriptorSetLayout(dsl_info);

		// Build the pipeline layout
		auto layout_info = vk::PipelineLayoutCreateInfo()
			.setPushConstantRanges(ranges);

		auto layout = device.createPipelineLayout(layout_info);

		// Compile the actual shaders
		auto vspv = link(vertex).generate(Target::spirv_binary_via_glsl, Stage::vertex);
		auto vmod_info = vk::ShaderModuleCreateInfo().setCode(vspv.template as <BinaryResult> ());
		auto vmodule = device.createShaderModule(vmod_info);

		auto fspv = link(fragment).generate(Target::spirv_binary_via_glsl, Stage::fragment);
		auto fmod_info = vk::ShaderModuleCreateInfo().setCode(fspv.template as <BinaryResult> ());
		auto fmodule = device.createShaderModule(fmod_info);

		// Configure the rest of the rasterization pipeline
		using vertex_inputs = filter_layout_in <vresources> ::group;
		using vertex_outputs = filter_layout_out <vresources> ::group;

		using fragment_inputs = filter_layout_in <fresources> ::group;
		using fragment_outputs = filter_layout_out <fresources> ::group;

		static constexpr bool layout_compatibility = verify_in_set_from_out_set(fragment_inputs(), vertex_outputs());

		static_assert(layout_compatibility,
			"fragment shader uses layout inputs which "
			"are not specified as vertex shader outputs");

		using sorted_vertex_inputs = decltype(sort_layout_inputs(vertex_inputs()));
		using vertex_t = decltype(vertex_storage_from_inputs(sorted_vertex_inputs()));

		auto shaders = std::array <vk::PipelineShaderStageCreateInfo, 2> {
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vmodule)
				.setPName("main"),
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fmodule)
				.setPName("main"),
		};

		auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptions(vertex_t::binding)
			.setVertexAttributeDescriptions(vertex_t::attributes);

		// TODO: possibly create a index buffer abstraction
		// which depends on this topology...
		auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		// TODO: options in the fragment REXEC context...
		auto multisampling_info = vk::PipelineMultisampleStateCreateInfo()
			.setRasterizationSamples(vk::SampleCountFlagBits::e1);

		// TODO: options in the vertex REXEC context
		auto rasterization_info = vk::PipelineRasterizationStateCreateInfo()
			.setCullMode(vk::CullModeFlagBits::eNone)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setLineWidth(1);

		auto dynamic_states = std::array <vk::DynamicState, 2> {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor,
		};

		auto dynamic_info = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStates(dynamic_states);

		auto viewport_info = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setScissorCount(1);

		// TODO: easier way to specify this from the fragment shader...
		auto blend_attachemnt = vk::PipelineColorBlendAttachmentState()
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setBlendEnable(true);

		auto blend_info = vk::PipelineColorBlendStateCreateInfo()
			.setBlendConstants({ 0, 0, 0, 0})
			.setAttachments(blend_attachemnt)
			.setLogicOp(vk::LogicOp::eCopy)
			.setLogicOpEnable(false);

		auto pipeline_info = vk::GraphicsPipelineCreateInfo()
			.setStages(shaders)
			.setLayout(layout)
			.setPVertexInputState(&vertex_input)
			.setPInputAssemblyState(&input_assembly)
			.setPMultisampleState(&multisampling_info)
			.setPRasterizationState(&rasterization_info)
			.setPDynamicState(&dynamic_info)
			.setPViewportState(&viewport_info)
			.setPColorBlendState(&blend_info)
			.setRenderPass(renderpass);

		auto pipeline = device.createGraphicsPipelines({}, pipeline_info).value;

		// TODO: return an object with correctly available set of operations
	}

	// Verification
	static constexpr bool verify() {
		// Granural checks which fail one at a time to prevent a wall of errors ;)

		// Basic check to ensure that we can work with these instances
		if constexpr (!procedure_class <V>)
			static_assert(false, "vertex program must be a procedure");
		else if constexpr (!procedure_class <F>)
			static_assert(false, "fragment program must be a procedure");
		
		// Ensure that both are actually entrypoints (based on signature)
		else if constexpr (!entrypoint_class <V>)
			static_assert(false, "vertex program must be an entrypoint (i.e. void())");
		else if constexpr (!entrypoint_class <F>)
			static_assert(false, "fragment program must be an entrypoint (i.e. void())");
		
		// Ensure that the entrypoints are from correct REXEC contexts
		else if constexpr (!vertex_class_entrypoint <V>)
			static_assert(false, "vertex entrypoint must be from a Vertex REXEC context");
		else if constexpr (!fragment_class_entrypoint <F>)
			static_assert(false, "fragment entrypoint must be from a Fragment REXEC context");

		// TODO: resource compatbility check

		return true;
	}

	static_assert(verify());
};

// TODO: automatic pipeline generation...
// TODO: remake solid_t <T> and start soft_t <T>

// TODO: tuple_storage which used leafs...

#include <littlevk/littlevk.hpp>

using V = decltype(VShader::main);
using vresources = V::rexec::collection;

using vertex_inputs = filter_layout_in <vresources> ::group;
using vertex_outputs = filter_layout_out <vresources> ::group;

using F = decltype(FShader::main);

int main()
{
	{
		auto glsl = link(VShader::main).generate_glsl();
		io::display_lines("GLSL", glsl);
		// VShader::main.display_assembly();
	}
	
	{
		auto glsl = link(FShader::main).generate_glsl();
		io::display_lines("GLSL", glsl);
		// FShader::main.display_assembly();
	}
	
	// Find a suitable physical device
	auto predicate = [&](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, {});
	};

	auto phdev = littlevk::pick_physical_device(predicate);

	auto device = littlevk::device(phdev, 0, 1, { VK_KHR_SWAPCHAIN_EXTENSION_NAME });
	auto deallocator = littlevk::Deallocator(device);

	vk::RenderPass renderpass = littlevk::RenderPassAssembler(device, deallocator)
		.add_attachment(littlevk::default_color_attachment(vk::Format::eR8G8B8A8Snorm))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	auto group = TraditionalPipelineGroup(VShader::main, FShader::main);
	group.compile(device, renderpass);
}
