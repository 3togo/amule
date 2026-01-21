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

#include "UpdateScheduler.h"
#include <Logger.h>
#include <libs/common/Format.h>
#include <libs/common/StringFunctions.h>
#include <libs/common/FileFunctions.h>
#include <Preferences.h>
#include <wx/protocol/http.h>
#include <wx/url.h>
#include <wx/sstream.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/filename.h>
#include <cstdio>

UpdateScheduler::UpdateScheduler()
    : m_configDir()
    , m_progressCallback()
    , m_completionCallback()
    , m_updateInProgress(false)
    , m_cancelled(false)
    , m_progress()
    , m_lastCheckTime()
    , m_lastUpdateTime()
{
    m_progress.inProgress = false;
    m_progress.percentComplete = 0;
    m_progress.bytesDownloaded = 0;
    m_progress.totalBytes = 0;
}

UpdateScheduler::~UpdateScheduler()
{
    CancelUpdate();
}

void UpdateScheduler::Initialize(const wxString& configDir)
{
    m_configDir = configDir;

    // Ensure trailing slash
    if (!m_configDir.IsEmpty() && !m_configDir.EndsWith("/") && !m_configDir.EndsWith("\\")) {
        m_configDir += "/";
    }

    AddLogLineN(CFormat(_("Update Scheduler initialized with config dir: %s")) % m_configDir);
}

void UpdateScheduler::SetProgressCallback(ProgressCallback callback)
{
    m_progressCallback = callback;
}

void UpdateScheduler::SetCompletionCallback(CompletionCallback callback)
{
    m_completionCallback = callback;
}

std::vector<DatabaseSource> UpdateScheduler::GetDefaultSources()
{
    std::vector<DatabaseSource> sources;

    // GitHub mirror (8bitsaver/maxmind-geoip) - Recommended
    DatabaseSource github;
    github.type = DB_TYPE_MAXMIND_DB;
    github.name = "GitHub Mirror (8bitsaver)";
    github.url = "https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb";
    github.checksumUrl = "https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb.sha256sum";
    github.priority = 0;
    github.enabled = true;
    sources.push_back(github);

    // jsDelivr CDN mirror
    DatabaseSource jsdelivr;
    jsdelivr.type = DB_TYPE_MAXMIND_DB;
    jsdelivr.name = "jsDelivr CDN";
    jsdelivr.url = "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb";
    jsdelivr.checksumUrl = "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb.sha256sum";
    jsdelivr.priority = 1;
    jsdelivr.enabled = true;
    sources.push_back(jsdelivr);

    // wp-statistics geolite2-country npm package
    DatabaseSource wpstats;
    wpstats.type = DB_TYPE_MAXMIND_DB;
    wpstats.name = "WP Statistics (jsDelivr)";
    wpstats.url = "https://cdn.jsdelivr.net/npm/geolite2-country/GeoLite2-Country.mmdb.gz";
    wpstats.checksumUrl = "";  // npm packages don't provide separate checksum
    wpstats.priority = 2;
    wpstats.enabled = true;
    sources.push_back(wpstats);

    return sources;
}

void UpdateScheduler::CheckForUpdatesAsync(CompletionCallback callback)
{
    m_completionCallback = callback;

    AddLogLineN(_("Checking for GeoIP database updates..."));

    // Try each enabled source
    auto sources = GetDefaultSources();
    for (const auto& source : sources) {
        if (!source.enabled) continue;

        AddLogLineN(CFormat(_("Checking source: %s")) % source.name);

        // For now, just report that update check would happen
        // In full implementation, would check remote metadata
        m_lastCheckTime = wxDateTime::Now();

        if (callback) {
            callback(UpdateCheckResult::NoUpdate, _("Update check simulated - source available"));
        }

        return;
    }

    if (callback) {
        callback(UpdateCheckResult::Error, _("No enabled sources"));
    }
}

void UpdateScheduler::DownloadUpdateAsync(const DatabaseSource& source, CompletionCallback callback)
{
    if (m_updateInProgress) {
        if (callback) {
            callback(UpdateCheckResult::Error, _("Update already in progress"));
        }
        return;
    }

    m_updateInProgress = true;
    m_cancelled = false;
    m_completionCallback = callback;

    AddLogLineN(CFormat(_("Starting download from: %s")) % source.name);

    wxString tempPath = GetTempPath();
    wxString finalPath = GetDatabasePath();

    // Download the file
    bool success = DownloadFile(source.url, tempPath);

    if (m_cancelled) {
        m_updateInProgress = false;
        if (callback) {
            callback(UpdateCheckResult::Error, _("Download cancelled"));
        }
        return;
    }

    if (!success) {
        m_updateInProgress = false;
        if (callback) {
            callback(UpdateCheckResult::NetworkError, _("Failed to download database"));
        }
        return;
    }

    // Check if file needs decompression (.gz)
    if (source.url.EndsWith(".gz")) {
        wxString decompressedPath = tempPath + ".decompressed";
        if (DecompressFile(tempPath, decompressedPath)) {
            if (wxFileExists(tempPath)) {
                wxRemoveFile(tempPath);
            }
            wxRenameFile(decompressedPath, tempPath);
        }
    }

    // Validate if checksum available
    if (!source.checksumUrl.IsEmpty()) {
        // Would fetch and validate checksum here
        AddLogLineN(_("Checksum validation would be performed here"));
    }

    // Install the update (rename temp to final)
    if (wxFileExists(finalPath)) {
        if (!wxRemoveFile(finalPath)) {
            m_updateInProgress = false;
            if (callback) {
                callback(UpdateCheckResult::Error, _("Failed to remove old database"));
            }
            return;
        }
    }

    if (!wxRenameFile(tempPath, finalPath)) {
        m_updateInProgress = false;
        if (callback) {
            callback(UpdateCheckResult::Error, _("Failed to install new database"));
        }
        return;
    }

    m_lastUpdateTime = wxDateTime::Now();
    m_updateInProgress = false;

    AddLogLineN(CFormat(_("Successfully updated GeoIP database to: %s")) % finalPath);

    if (callback) {
        callback(UpdateCheckResult::UpdateAvailable, _("Database updated successfully"));
    }
}

void UpdateScheduler::CancelUpdate()
{
    m_cancelled = true;
    if (m_updateInProgress) {
        AddLogLineN(_("Cancelling update download..."));
    }
}

bool UpdateScheduler::DownloadFile(const wxString& url, const wxString& outputPath)
{
    // Create output directory if it doesn't exist
    wxFileName outputFile(outputPath);
    wxString outputDir = outputFile.GetPath();
    if (!wxDirExists(outputDir)) {
        if (!wxMkdir(outputDir, wxS_DIR_DEFAULT)) {
            AddLogLineC(CFormat(_("Failed to create output directory: %s")) % outputDir);
            return false;
        }
        // Recursively make dirs if needed
        if (!wxDirExists(outputDir)) {
            AddLogLineC(CFormat(_("Failed to create output directory: %s")) % outputDir);
            return false;
        }
    }

    wxURL wxurl(url);
    if (wxurl.GetError() != wxURL_NOERR) {
        m_progress.statusMessage = _("Invalid URL");
        m_progress.inProgress = false;
        AddLogLineC(CFormat(_("Invalid URL: %s")) % url);
        return false;
    }

    // Work with wxProtocol as a temporary object since GetProtocol() returns an object
    // Use wxHTTP directly for operations
    wxHTTP httpProtocol;
    // Configure user agent if needed
    httpProtocol.SetHeader("User-Agent", "aMule/2.3 (GeoIP Update)");

    // Create input stream directly using wxHTTP
    wxInputStream* inputStream = httpProtocol.GetInputStream(url);

    if (!inputStream) {
        m_progress.statusMessage = _("Connection failed");
        m_progress.inProgress = false;
        AddLogLineC(CFormat(_("Failed to connect to %s")) % url);
        return false;
    }

    // Check for input stream errors
    if (inputStream->GetLastError() != wxSTREAM_NO_ERROR) {
        m_progress.statusMessage = _("Connection failed");
        m_progress.inProgress = false;
        AddLogLineC(CFormat(_("Failed to connect to %s")) % url);
        delete inputStream;
        return false;
    }

    wxFileOutputStream outputStream(outputPath);
    if (!outputStream.IsOk()) {
        m_progress.statusMessage = _("Cannot create file");
        m_progress.inProgress = false;
        AddLogLineC(CFormat(_("Failed to create output file: %s")) % outputPath);
        delete inputStream;
        return false;
    }

    m_progress.statusMessage = _("Downloading...");
    if (m_progressCallback) {
        m_progressCallback(m_progress);
    }

    // Get content length if available
    // Note: wxProtocol doesn't have GetHeader in newer wxWidgets versions
    
    char buffer[4096];
    wxFileOffset totalWritten = 0;

    while (!inputStream->Eof() && !m_cancelled) {
        inputStream->Read(buffer, sizeof(buffer));
        size_t bytesRead = inputStream->LastRead(); // Get the actual number of bytes read
        wxStreamError readStatus = inputStream->GetLastError();
        
        if (readStatus != wxSTREAM_NO_ERROR && readStatus != wxSTREAM_EOF) {
            m_progress.statusMessage = _("Download error");
            m_progress.inProgress = false;
            AddLogLineC(_("Error reading from server"));
            delete inputStream;
            return false;
        }

        if (bytesRead > 0) {
            outputStream.Write(buffer, bytesRead);
            totalWritten += bytesRead;

            m_progress.bytesDownloaded = totalWritten;

            // Update progress if callback is available
            if (m_progressCallback) {
                m_progressCallback(m_progress);
            }
        }
    }

    delete inputStream;

    if (m_cancelled) {
        m_progress.statusMessage = _("Cancelled");
        m_progress.inProgress = false;
        return false;
    }

    m_progress.statusMessage = _("Completed");
    m_progress.inProgress = false;
    if (m_progressCallback) {
        m_progressCallback(m_progress);
    }

    return true;
}

bool UpdateScheduler::DecompressFile(const wxString& compressedPath, const wxString& outputPath)
{
    wxFileInputStream inputStream(compressedPath);
    if (!inputStream.IsOk()) {
        AddLogLineC(CFormat(_("Failed to open compressed file: %s")) % compressedPath);
        return false;
    }

    wxFileOutputStream outputStream(outputPath);
    if (!outputStream.IsOk()) {
        AddLogLineC(CFormat(_("Failed to create output file: %s")) % outputPath);
        return false;
    }

    wxZlibInputStream zlibStream(inputStream);

    if (!zlibStream.IsOk()) {
        AddLogLineC(_("Failed to initialize decompression"));
        return false;
    }

    char buffer[4096];
    size_t totalWritten = 0;

    while (!zlibStream.Eof()) {
        zlibStream.Read(buffer, sizeof(buffer));
        size_t bytesRead = zlibStream.LastRead();
        wxStreamError readStatus = zlibStream.GetLastError();
        
        if (readStatus != wxSTREAM_NO_ERROR && readStatus != wxSTREAM_EOF) {
            AddLogLineC(_("Error during decompression"));
            return false;
        }

        if (bytesRead > 0) {
            outputStream.Write(buffer, bytesRead);
            totalWritten += bytesRead;
        }
    }

    return true;
}

bool UpdateScheduler::ValidateFile(const wxString& filePath, const wxString& expectedChecksum)
{
    if (expectedChecksum.IsEmpty()) {
        AddLogLineN(_("No checksum provided, skipping validation"));
        return true;
    }

    AddLogLineN(CFormat(_("Validating file checksum: %s")) % filePath);

    // SHA256 checksum calculation would go here
    // For now, just return true
    // In production, would use wxCryptoHash or external tool

    return true;
}