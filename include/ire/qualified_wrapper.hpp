#pragma once

#include "../thunder/enumerations.hpp"
#include "concepts.hpp"

namespace jvl::ire {

using qualifier_extension_t = thunder::Index (*)(thunder::Index);

template <generic, thunder::QualifierKind, qualifier_extension_t>
struct qualified_wrapper {};

// Wrapper for builtin types
template <builtin T>
struct builtin_wrapper {
	T value;

	template <typename ... Args>
	builtin_wrapper(const Args &... args) : value(args...) {}

	auto layout() {
		static const std::string name = fmt::format("Wrapper{}", type_counter <T> ());
		return phantom_layout_from(name, verbatim_field(value));
	}
};

// Implementation built-ins
template <builtin T, thunder::QualifierKind K, qualifier_extension_t Ext>
class qualified_wrapper <T, K, Ext> : public T {
private:
	size_t binding;
protected:
	template <typename ... Args>
	qualified_wrapper(cache_index_t source, thunder::ConstructorMode mode, size_t binding_, const Args &... args)
			: T(args...), binding(binding_) {
		auto &em = Emitter::active;

		builtin_wrapper <T> wrapper(args...);

		auto layout = wrapper.layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, binding, K);
		
		auto extd = qual;
		if (Ext)
			extd = Ext(extd);

		auto value = em.emit_construct(extd, source.id, mode);
		wrapper.layout().link(value);
		this->ref = wrapper.value.ref;
	}
public:
	using base = T;
	static constexpr auto qualifier = K;
	static inline qualifier_extension_t extension = Ext;

	template <typename ... Args>
	qualified_wrapper(size_t binding_, const Args &... args)
			: qualified_wrapper(cache_index_t::from(-1),
				thunder::transient,
				binding_,
				args...) {}

	operator typename T::arithmetic_type() const {
		return arithmetic_type(T::synthesize());
	}
};

// Implementation for aggregate types
template <aggregate T, thunder::QualifierKind K, qualifier_extension_t Ext>
class qualified_wrapper <T, K, Ext> : public T {
private:
	size_t binding;
protected:
	template <typename ... Args>
	qualified_wrapper(cache_index_t source, thunder::ConstructorMode mode, size_t binding_, const Args &... args)
			: T(args...), binding(binding_) {
		auto &em = Emitter::active;

		auto layout = this->layout();
		auto type = layout.generate_type().concrete();
		auto qual = em.emit_qualifier(type, binding, K);

		auto extd = qual;
		if (Ext)
			extd = Ext(extd);

		auto value = em.emit_construct(extd, source.id, mode);
		layout.link(value);
	}
public:
	using base = T;
	static constexpr auto qualifier = K;
	static inline qualifier_extension_t extension = Ext;

	template <typename ... Args>
	qualified_wrapper(size_t binding_, const Args &... args)
			: qualified_wrapper(cache_index_t::from(-1),
				thunder::transient,
				binding_,
				args...) {}
};

// TODO: bindless qualifier

} // namespace jvl::ire
