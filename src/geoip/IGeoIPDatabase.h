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

#ifndef IGEODATABASE_H
#define IGEODATABASE_H

#include <wx/string.h>

/**
 * @brief Supported database formats
 */
enum DatabaseType {
    DB_TYPE_UNKNOWN = 0,
    DB_TYPE_LEGACY_GEOIP,   ///< Legacy GeoIP.dat format
    DB_TYPE_MAXMIND_DB,     ///< MaxMind DB format (.mmdb)
    DB_TYPE_CSV,            ///< CSV format
    DB_TYPE_SQLITE          ///< SQLite format (future)
};

/**
 * @brief Interface for GeoIP database implementations
 */
class IGeoIPDatabase
{
public:
    virtual ~IGeoIPDatabase() {}

    /**
     * @brief Open the database file
     * @param path Path to the database file
     * @return true if successful, false otherwise
     */
    virtual bool Open(const wxString& path) = 0;

    /**
     * @brief Close the database
     */
    virtual void Close() = 0;

    /**
     * @brief Get country code for IP address
     * @param ip IP address string
     * @return 2-letter country code or empty string if not found
     */
    virtual wxString GetCountryCode(const wxString& ip) = 0;

    /**
     * @brief Get country name for IP address
     * @param ip IP address string
     * @return Country name or empty string if not found
     */
    virtual wxString GetCountryName(const wxString& ip) = 0;

    /**
     * @brief Check if database is valid and ready
     * @return true if database is valid
     */
    virtual bool IsValid() const = 0;

    /**
     * @brief Get database type
     * @return Database type
     */
    virtual DatabaseType GetType() const = 0;

    /**
     * @brief Get database format name
     * @return Human-readable format name
     */
    virtual wxString GetFormatName() const = 0;

    /**
     * @brief Get database version
     * @return Version string
     */
    virtual wxString GetVersion() const = 0;

    /**
     * @brief Get database description
     * @return Description string
     */
    virtual wxString GetDescription() const = 0;
};

#endif // IGEODATABASE_H