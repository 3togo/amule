// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#include "CompletedFileDeletion.h"

#include <common/Path.h>
#include <common/StringFunctions.h>
#include <filesystem>
#include <system_error>

bool CompletedFilePathIsMissing(const CPath &path)
{
#ifdef __WINDOWS__
	const std::filesystem::path nativePath(path.GetRaw().ToStdWstring());
#else
	const auto nativeName = filename2char(path.GetRaw());
	if (!nativeName.data() || nativeName.length() == 0) {
		return false;
	}
	const std::filesystem::path nativePath(nativeName.data());
#endif
	std::error_code error;
	const auto status = std::filesystem::symlink_status(nativePath, error);
	return status.type() == std::filesystem::file_type::not_found &&
	       (!error || error == std::errc::no_such_file_or_directory);
}
