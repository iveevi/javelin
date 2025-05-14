#pragma once

// Broad definitions and utilities
#include "rexec/class.hpp"
#include "rexec/collection.hpp"
#include "rexec/manifestation.hpp"
#include "rexec/resources.hpp"
#include "rexec/validation.hpp"
#include "rexec/error.hpp"

// Specialized REXECs
#include "rexec/context/callable.hpp"
#include "rexec/context/fragment.hpp"
#include "rexec/context/vertex.hpp"

// Pipeline definitions and interface
#include "rexec/pipeline/specifications.hpp"
#include "rexec/pipeline/indicators.hpp"

// Static pipeline verification
#include "rexec/verification/entrypoints_status.hpp"
#include "rexec/verification/entrypoints_traditional.hpp"
#include "rexec/verification/entrypoints.hpp"

#include "rexec/verification/compilation_status.hpp"
#include "rexec/verification/compilation_results.hpp"

#include "rexec/verification/compilation_traditional.hpp"
#include "rexec/verification/compilation_traditional_verification.hpp"
#include "rexec/verification/compilation_traditional_vulkan.hpp"

#include "rexec/verification/deduction.hpp"