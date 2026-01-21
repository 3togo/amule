//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include "IGeoIPDatabase.h"
#include <wx/string.h>
#include <memory>

/**
 * @brief Database source configuration
 */
struct DatabaseSource {
    DatabaseType type;        ///< Database format type
    wxString name;           ///< Human-readable name
    wxString url;           ///< Download URL
    wxString checksumUrl;   ///< Checksum URL for verification
    int priority;           ///< Priority (lower = higher priority)
    bool enabled;           ///< Whether this source is enabled

    DatabaseSource() : type(DB_TYPE_UNKNOWN), priority(100), enabled(true) {}
};

/**
 * @brief Factory for creating GeoIP database instances
 */
class DatabaseFactory
{
public:
    /**
     * @brief Create a database instance for the given type
     * @param type Database type
     * @return Shared pointer to database instance
     */
    static std::shared_ptr<IGeoIPDatabase> CreateDatabase(DatabaseType type);

    /**
     * @brief Create a database instance from file (auto-detection)
     * @param path Path to database file
     * @return Shared pointer to database instance or nullptr if failed
     */
    static std::shared_ptr<IGeoIPDatabase> CreateFromFile(const wxString& path);

    /**
     * @brief Detect database format from file
     * @param path Path to database file
     * @return Detected database type
     */
    static DatabaseType DetectFormat(const wxString& path);

    /**
     * @brief Get file extension for database type
     * @param type Database type
     * @return File extension with dot (e.g., ".mmdb")
     */
    static wxString GetFileExtension(DatabaseType type);

    /**
     * @brief Get supported database types
     * @return Vector of supported database types
     */
    static std::vector<DatabaseType> GetSupportedTypes();

    /**
     * @brief Check if database type is supported
     * @param type Database type
     * @return true if supported
     */
    static bool IsSupported(DatabaseType type);
};

#endif // DATABASEFACTORY_H