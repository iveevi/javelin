#pragma once

#include "qualified_type.hpp"

namespace jvl::thunder {

// Overload for primitive type operations/intrinsics
struct overload {
	QualifiedType result;
	std::vector <QualifiedType> args;

	template <typename ... Args>
	static overload from(PrimitiveType result, const Args &...args) {
		std::vector <QualifiedType> list { QualifiedType::primitive(args)... };
		return overload(QualifiedType::primitive(result), list);
	}

	template <typename ... Args>
	static overload from(const QualifiedType &result, const Args &...args) {
		std::vector <QualifiedType> list { args... };
		return overload(result, list);
	}

	static std::string type_decls_to_string(const std::vector <QualifiedType> &args) {
		std::string result = "(";
		for (size_t i = 0; i < args.size(); i++) {
			auto &td = args[i];

			result += td.to_string();
			if (i + 1 < args.size())
				result += ", ";
		}

		return result + ")";
	}
};
	
using overload_list = std::vector <overload>;

inline overload_list concat(const overload_list &A, const overload_list &B)
{
	overload_list C = A;
	for (auto &ovl : B)
		C.push_back(ovl);
	return C;
}

template <typename T>
struct overload_table : public wrapped::hash_table <T, overload_list> {
	using wrapped::hash_table <T, overload_list> ::hash_table;

	QualifiedType lookup(const T &key, const std::vector <QualifiedType> &args) const {
		MODULE(overload-table);

		const char *const *name_table = nullptr;
		if constexpr (std::same_as <T, IntrinsicOperation>)
			name_table = tbl_intrinsic_operation;
		else if constexpr (std::same_as <T, OperationCode>)
			name_table = tbl_operation_code;

		JVL_ASSERT(this->contains(key),
			"overload table does not contain entry for ${}",
			name_table[key]);
		
		auto &overloads = this->at(key);
		for (auto &ovl : overloads) {
			if (ovl.args.size() != args.size())
				continue;

			bool success = true;
			for (size_t i = 0; i < args.size(); i++) {
				if (ovl.args[i] != args[i]) {
					success = false;
					break;
				}
			}

			if (success)
				return ovl.result;
		}

		JVL_ERROR("no matching overload {} found for ${}",
			overload::type_decls_to_string(args),
			name_table[key]);

		JVL_NOTE("available overloads for ${}:", name_table[key]);
		for (auto &ovl : overloads) {
			fmt::println("  {} -> {}",
				overload::type_decls_to_string(ovl.args),
				ovl.result.to_string());
		}

		return QualifiedType();
	}
};

// Overload lookup methods for oeprations and intrinsics
QualifiedType lookup_intrinsic_overload(const IntrinsicOperation &, const std::vector <QualifiedType> &);
QualifiedType lookup_operation_overload(const OperationCode &, const std::vector <QualifiedType> &);

} // namespace jvl::thunder