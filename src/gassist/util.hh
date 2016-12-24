#pragma once

namespace gassist {

/// Type signifying empty values; e.g. used for
/// specifically constructing an empty object to
/// move into later
struct empty_t {};

/// Value of empty_t
constexpr empty_t empty;

} // ns gassist
