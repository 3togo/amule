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

#include "DatabaseFactory.h"
#include "geoip/MaxMindDBDatabase.h"
#include <Logger.h>
#include <libs/common/StringFunctions.h>
#include <libs/common/Format.h>  // Added for CFormat
#include <wx/filename.h>

std::shared_ptr<IGeoIPDatabase> DatabaseFactory::CreateDatabase(DatabaseType type)
{
    switch (type) {
        case DB_TYPE_MAXMIND_DB:
            return std::make_shared<MaxMindDBDatabase>();

        // Legacy GeoIP not supported per user requirements
        case DB_TYPE_LEGACY_GEOIP:
            AddLogLineN(_("Legacy GeoIP format is no longer supported"));
            return nullptr;

        case DB_TYPE_CSV:
            // TODO: Implement CSV support
            AddLogLineN(_("CSV format support not yet implemented"));
            return nullptr;

        case DB_TYPE_SQLITE:
            // TODO: Implement SQLite support
            AddLogLineN(_("SQLite format support not yet implemented"));
            return nullptr;

        default:
            AddLogLineC(CFormat(_("Unknown database type: %d")) % static_cast<int>(type));
            return nullptr;
    }
}

std::shared_ptr<IGeoIPDatabase> DatabaseFactory::CreateFromFile(const wxString& path)
{
    if (path.IsEmpty()) {
        return nullptr;
    }

    DatabaseType type = DetectFormat(path);
    if (type == DB_TYPE_UNKNOWN) {
        AddLogLineC(CFormat(_("Could not detect database format for: %s")) % path);
        return nullptr;
    }

    auto database = CreateDatabase(type);
    if (!database) {
        return nullptr;
    }

    if (!database->Open(path)) {
        AddLogLineC(CFormat(_("Failed to open database: %s")) % path);
        return nullptr;
    }

    return database;
}

DatabaseType DatabaseFactory::DetectFormat(const wxString& path)
{
    wxFileName filename(path);

    if (!filename.FileExists()) {
        return DB_TYPE_UNKNOWN;
    }

    wxString ext = filename.GetExt().Lower();

    // Check file extension first
    if (ext == "mmdb") {
        return DB_TYPE_MAXMIND_DB;
    }

    if (ext == "dat") {
        // Legacy GeoIP.dat - not supported
        return DB_TYPE_LEGACY_GEOIP;
    }

    if (ext == "csv") {
        return DB_TYPE_CSV;
    }

    if (ext == "db" || ext == "sqlite") {
        return DB_TYPE_SQLITE;
    }

    // Try to detect by reading file header
    FILE* fp = wxFopen(path, "rb");
    if (!fp) {
        return DB_TYPE_UNKNOWN;
    }

    unsigned char header[16];
    size_t read = fread(header, 1, sizeof(header), fp);
    fclose(fp);

    if (read < 4) {
        return DB_TYPE_UNKNOWN;
    }

    // MaxMind DB magic bytes: 0xDB 0xEE 0x47 0x0F
    if (header[0] == 0xDB && header[1] == 0xEE && header[2] == 0x47 && header[3] == 0x0F) {
        return DB_TYPE_MAXMIND_DB;
    }

    // Legacy GeoIP: might start with GeoIP Country V6
    if (memcmp(header, "GeoIP Country", 13) == 0) {
        return DB_TYPE_LEGACY_GEOIP;
    }

    return DB_TYPE_UNKNOWN;
}

wxString DatabaseFactory::GetFileExtension(DatabaseType type)
{
    switch (type) {
        case DB_TYPE_MAXMIND_DB:
            return ".mmdb";
        case DB_TYPE_LEGACY_GEOIP:
            return ".dat";
        case DB_TYPE_CSV:
            return ".csv";
        case DB_TYPE_SQLITE:
            return ".db";
        default:
            return wxEmptyString;
    }
}

std::vector<DatabaseType> DatabaseFactory::GetSupportedTypes()
{
    return {
        DB_TYPE_MAXMIND_DB,
        DB_TYPE_CSV
    };
}

bool DatabaseFactory::IsSupported(DatabaseType type)
{
    return type == DB_TYPE_MAXMIND_DB || type == DB_TYPE_CSV;
}