// Copyright (c) 2026 aMule Team
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef COMPLETEDFILEDELETION_H
#define COMPLETEDFILEDELETION_H

class CPath;
// False for inaccessible paths, directories, and dangling symbolic links.
// Only a confirmed missing directory entry may count as an already deleted file.
bool CompletedFilePathIsMissing(const CPath &path);

#endif
