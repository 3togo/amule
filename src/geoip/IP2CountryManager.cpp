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

#include "IP2CountryManager.h"
#include "src/pixmaps/flags_xpm/CountryFlags.h"
#include <Logger.h>
#include <common/Format.h>
#include <common/FileFunctions.h>
#include <CFile.h>
#include <Preferences.h>
#include <wx/utils.h>         // For wxExecute
#include <wx/filefn.h>        // For wxFileExists, wxRenameFile
#include <wx/dir.h>           // For wxDir

// Static member initialization
std::unique_ptr<IP2CountryManager> IP2CountryManager::m_instance = nullptr;

// Constructor
IP2CountryManager::IP2CountryManager()
    : m_configDir()
    , m_databasePath()
    , m_database(nullptr)
    , m_scheduler(nullptr)
    , m_enabled(false)
    , m_autoUpdateEnabled(true)
    , m_updateCheckDays(7)
{
}

// Destructor
IP2CountryManager::~IP2CountryManager()
{
    Disable();
}

IP2CountryManager& IP2CountryManager::GetInstance()
{
    if (!m_instance) {
        m_instance = std::make_unique<IP2CountryManager>();
    }
    return *m_instance;
}

void IP2CountryManager::DestroyInstance()
{
    m_instance.reset();
}

bool IP2CountryManager::Initialize(const wxString& configDir)
{
    m_configDir = configDir;

    // Ensure trailing slash
    if (!m_configDir.IsEmpty() &&
        !m_configDir.EndsWith("/") &&
        !m_configDir.EndsWith("\\")) {
        m_configDir += "/";
    }

    // Initialize update scheduler
    m_scheduler = std::make_unique<UpdateScheduler>();
    m_scheduler->Initialize(m_configDir);

    // Set up callbacks
    m_scheduler->SetProgressCallback(
        [this](const UpdateProgress& progress) {
            OnUpdateProgress(progress);
        }
    );

    m_scheduler->SetCompletionCallback(
        [this](UpdateCheckResult result, const wxString& message) {
            OnUpdateComplete(result, message);
        }
    );

    // Set default database path
    m_databasePath = m_configDir + "GeoLite2-Country.mmdb";

    AddLogLineN(CFormat(_("IP2Country Manager initialized")));

    // Try to load existing database
    return LoadDatabase();
}

void IP2CountryManager::Enable()
{
    if (m_enabled) {
        return;
    }

    if (!m_database) {
        // Try to load database
        if (!LoadDatabase()) {
            AddLogLineC(_("IP2Country: Failed to load database, leaving disabled"));
            return;
        }
    }

    // Load country flags
    if (m_CountryDataMap.empty()) {
        LoadFlags();
    }

    m_enabled = true;
    AddLogLineN(_("IP2Country: Enabled"));
}

void IP2CountryManager::Disable()
{
    if (!m_enabled) {
        return;
    }

    m_enabled = false;
    m_database.reset();

    AddLogLineN(_("IP2Country: Disabled"));
}

CountryDataNew IP2CountryManager::GetCountryData(const wxString& ip)
{
    CountryDataNew result;

    if (!m_enabled || !m_database || !m_database->IsValid()) {
        // Return unknown country
        result.Code = "unknown";
        result.Name = "?";
        return result;
    }

    if (ip.IsEmpty()) {
        result.Code = "unknown";
        result.Name = "?";
        return result;
    }

    // Get country code from database
    wxString countryCode = m_database->GetCountryCode(ip);

    if (countryCode.IsEmpty()) {
        countryCode = "unknown";
    }

    // Look up in our flag map
    auto it = m_CountryDataMap.find(countryCode.Lower());
    if (it != m_CountryDataMap.end()) {
        result = it->second;
    } else {
        // Unknown country code
        result.Code = countryCode.Lower();
        result.Name = countryCode.Upper();

        // Try to find unknown flag
        auto unknownIt = m_CountryDataMap.find("unknown");
        if (unknownIt != m_CountryDataMap.end()) {
            result.Flag = unknownIt->second.Flag;
        }
    }

    return result;
}

wxString IP2CountryManager::GetCountryCode(const wxString& ip)
{
    if (!m_enabled || !m_database) {
        return wxEmptyString;
    }

    return m_database->GetCountryCode(ip);
}

wxString IP2CountryManager::GetCountryName(const wxString& ip)
{
    if (!m_enabled || !m_database) {
        return wxEmptyString;
    }

    return m_database->GetCountryName(ip);
}

void IP2CountryManager::CheckForUpdates()
{
    if (!m_scheduler) {
        AddLogLineC(_("Update scheduler not initialized"));
        return;
    }

    m_scheduler->CheckForUpdatesAsync();
}

void IP2CountryManager::DownloadUpdate()
{
    if (!m_scheduler) {
        AddLogLineC(_("Update scheduler not initialized"));
        return;
    }

    auto sources = UpdateScheduler::GetDefaultSources();
    for (const auto& source : sources) {
        if (source.enabled) {
            m_scheduler->DownloadUpdateAsync(source);
            return;
        }
    }

    AddLogLineC(_("No enabled update sources"));
}

bool IP2CountryManager::Reload()
{
    return LoadDatabase();
}

void IP2CountryManager::SetDatabasePath(const wxString& path)
{
    if (m_databasePath == path) {
        return;
    }

    m_databasePath = path;

    // Reload if enabled
    if (m_enabled) {
        LoadDatabase();
    }
}

void IP2CountryManager::SetUpdateCheckInterval(int days)
{
    m_updateCheckDays = std::max(1, std::min(days, 30));
}

void IP2CountryManager::SetAutoUpdateEnabled(bool enabled)
{
    m_autoUpdateEnabled = enabled;

    if (enabled) {
        AddLogLineN(_("IP2Country: Auto-update enabled"));
    } else {
        AddLogLineN(_("IP2Country: Auto-update disabled"));
    }
}

void IP2CountryManager::LoadFlags()
{
    m_CountryDataMap.clear();

    // Load data from xpm files
    for (int i = 0; i < flags::FLAGS_XPM_SIZE; ++i) {
        CountryDataNew countrydata;
        countrydata.Code = wxString(flags::flagXPMCodeVector[i].code, wxConvISO8859_1);
        countrydata.Flag = wxImage(flags::flagXPMCodeVector[i].xpm);
        countrydata.Name = countrydata.Code;

        if (countrydata.Flag.IsOk()) {
            m_CountryDataMap[countrydata.Code] = countrydata;
        } else {
            AddLogLineC(CFormat(_("Failed to load flag for country code: %s")) % countrydata.Code);
        }
    }

    AddDebugLogLineN(logGeneral,
        CFormat(wxT("IP2Country: Loaded %d country flags")) % m_CountryDataMap.size());
}

bool IP2CountryManager::LoadDatabase()
{
    // Try to open the database
    auto result = DatabaseFactory::CreateAndOpen(m_databasePath);
    m_database = result.database;

    if (!m_database) {
        // Database doesn't exist - try to find one in config dir
        wxDir dir(m_configDir);
        if (dir.IsOpened()) {
            wxString filename;
            bool cont = dir.GetFirst(&filename, "*.mmdb", wxDIR_FILES);
            while (cont) {
                wxString fullPath = m_configDir + filename;
                auto searchResult = DatabaseFactory::CreateAndOpen(fullPath);
                if (searchResult.database) {
                    m_database = searchResult.database;
                    m_databasePath = fullPath;
                    AddLogLineN(CFormat(_("Found database at: %s")) % fullPath);
                    break;
                }
                cont = dir.GetNext(&filename);
            }
        }
    }

    if (!m_database) {
        AddLogLineN(CFormat(_("No GeoIP database found at: %s")) % m_databasePath);
        
        // Attempt automatic download
        if (!DownloadDatabase()) {
            AddLogLineN(_("Failed to download GeoIP database automatically."));
            AddLogLineN(_("You can download the GeoLite2-Country database from:"));
            AddLogLineN(_("  - https://github.com/8bitsaver/maxmind-geoip"));
            return false;
        }
    }

    AddLogLineN(CFormat(_("IP2Country: Loaded database from %s")) % m_databasePath);
    AddLogLineN(CFormat(_("IP2Country: Database version: %s")) % m_database->GetVersion());
    AddLogLineN(CFormat(_("IP2Country: Database format: %s")) % m_database->GetFormatName());

    return true;
}

void IP2CountryManager::OnUpdateProgress(const UpdateProgress& progress)
{
    if (progress.inProgress) {
        wxString status = CFormat(_("Downloading GeoIP: %d%% (%s / %s)")) %
            progress.percentComplete %
            CastItoXBytes(progress.bytesDownloaded) %
            CastItoXBytes(progress.totalBytes);

        AddLogLineN(status);
    } else {
        AddLogLineN(progress.statusMessage);
    }
}

void IP2CountryManager::OnUpdateComplete(UpdateCheckResult result, const wxString& message)
{
    switch (result) {
        case UpdateCheckResult::UpdateAvailable:
            AddLogLineN(CFormat(_("Update available: %s")) % message);
            break;

        case UpdateCheckResult::NoUpdate:
            AddLogLineN(CFormat(_("No update available: %s")) % message);
            break;

        case UpdateCheckResult::NetworkError:
            AddLogLineC(CFormat(_("Network error during update: %s")) % message);
            break;

        case UpdateCheckResult::Error:
        default:
            AddLogLineC(CFormat(_("Update error: %s")) % message);
            break;
    }

    // If update was successful, reload the database
    if (result == UpdateCheckResult::UpdateAvailable) {
        if (Reload()) {
            AddLogLineN(_("IP2Country: Database reloaded after update"));
        }
    }
}

bool IP2CountryManager::DownloadDatabase()
{
    AddLogLineN(_("Attempting to download GeoLite2-Country database automatically..."));
    
    // Try to download the database
    UpdateProgress progress;
    progress.inProgress = true;
    progress.percentComplete = 0;
    progress.statusMessage = _("Starting download...");
    OnUpdateProgress(progress);
    
    // Download the database synchronously
    wxString tempPath = m_configDir + "GeoLite2-Country.mmdb.temp";
    wxString downloadUrl = "https://cdn.jsdelivr.net/npm/geolite2-country@1.0.2/GeoLite2-Country.mmdb.gz";
    
    bool downloadSuccess = false;
    
    // Use wget to download the database
    wxString command = wxString::Format("wget -q --show-progress -O \"%s.gz\" \"%s\"", tempPath, downloadUrl);
    int result = wxExecute(command, wxEXEC_SYNC);
    
    if (result == 0) {
        // Extract the gzip file
        wxString extractCommand = wxString::Format("gunzip -f \"%s.gz\"", tempPath);
        result = wxExecute(extractCommand, wxEXEC_SYNC);
        
        if (result == 0 && wxFileExists(tempPath)) {
            // Move to final location
            if (wxRenameFile(tempPath, m_databasePath)) {
                AddLogLineN(_("Database downloaded successfully!"));
                downloadSuccess = true;
                
                // Try to load the downloaded database
                auto loadResult = DatabaseFactory::CreateAndOpen(m_databasePath);
                m_database = loadResult.database;
            }
        }
    }
    
    progress.percentComplete = 100;
    progress.statusMessage = _("Download completed");
    OnUpdateProgress(progress);
    
    return downloadSuccess && m_database != nullptr;
}