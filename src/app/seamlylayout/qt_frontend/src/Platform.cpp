// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file Platform.cpp
// @brief Static member definition for Platform.

#include "Platform.h"

// Default to Linux; overwritten by Platform::init() at startup.
Platform::OS Platform::os = Platform::Linux;
