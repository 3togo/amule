//
// GeoIP Database Download Helper for aMule
// This tool helps users download and install the required GeoIP database
//

#include <wx/wx.h>
#include <wx/url.h>
#include <wx/wfstream.h>
#include <wx/filename.h>
#include <wx/progdlg.h>
#include <wx/sstream.h>
#include <wx/zstream.h>
#include <iostream>
#include <vector>

class GeoIPDownloadHelper : public wxApp
{
public:
    virtual bool OnInit();
    
private:
    bool DownloadDatabase(const wxString& url);
    bool ExtractIfNeeded(const wxString& downloadedFile);
    bool VerifyDatabase(const wxString& databasePath);
    void ShowHelp();
    
    wxString m_configDir;
};

bool GeoIPDownloadHelper::OnInit()
{
    // Parse command line arguments
    wxCmdLineParser parser(argc, argv);
    parser.AddSwitch("h", "help", "Show this help message");
    parser.AddOption("d", "dir", "aMule config directory", wxCMD_LINE_VAL_STRING);
    
    if (parser.Parse() != 0 || parser.Found("h")) {
        ShowHelp();
        return false;
    }
    
    // Get config directory
    if (!parser.Found("d", &m_configDir)) {
        m_configDir = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + ".aMule";
    }
    
    wxPrintf("GeoIP Database Download Helper for aMule\n");
    wxPrintf("========================================\n\n");
    
    // Check if directory exists
    if (!wxDirExists(m_configDir)) {
        wxPrintf("Error: Config directory does not exist: %s\n", m_configDir);
        wxPrintf("Please specify the correct aMule config directory with -d option\n");
        return false;
    }
    
    wxPrintf("Using config directory: %s\n\n", m_configDir);
    
    // Available download sources
    std::vector<wxString> sources = {
        "https://raw.githubusercontent.com/8bitsaver/maxmind-geoip/release/GeoLite2-Country.mmdb",
        "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb",
        "https://cdn.jsdelivr.net/npm/geolite2-country/GeoLite2-Country.mmdb.gz"
    };
    
    wxPrintf("Available download sources:\n");
    for (size_t i = 0; i < sources.size(); ++i) {
        wxPrintf("%zu. %s\n", i + 1, sources[i]);
    }
    wxPrintf("\n");
    
    // Ask user to choose a source
    wxString input;
    wxPrintf("Choose a download source (1-%zu, or 0 to exit): ", sources.size());
    std::getline(std::cin, input);
    
    long choice = 0;
    if (!input.ToLong(&choice) || choice < 1 || choice > sources.size()) {
        wxPrintf("Exiting.\n");
        return false;
    }
    
    wxString selectedUrl = sources[choice - 1];
    wxPrintf("Selected: %s\n\n", selectedUrl);
    
    // Download the database
    if (DownloadDatabase(selectedUrl)) {
        wxPrintf("\n✅ Download successful! GeoIP database is now ready.\n");
        wxPrintf("Restart aMule to enable country flag display.\n");
    } else {
        wxPrintf("\n❌ Download failed. Please try another source or check your internet connection.\n");
    }
    
    return false;
}

bool GeoIPDownloadHelper::DownloadDatabase(const wxString& url)
{
    wxString outputPath = m_configDir + wxFileName::GetPathSeparator() + "GeoLite2-Country.mmdb";
    wxString tempPath = outputPath + ".download";
    
    wxPrintf("Downloading from: %s\n", url);
    wxPrintf("Saving to: %s\n", outputPath);
    
    // Create progress dialog
    wxProgressDialog progressDialog(
        "Downloading GeoIP Database",
        "Connecting to server...",
        100,
        NULL,
        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME
    );
    
    wxURL webUrl(url);
    if (!webUrl.GetError().IsOk()) {
        wxPrintf("Error: Invalid URL\n");
        return false;
    }
    
    wxInputStream* httpStream = webUrl.GetInputStream();
    if (!httpStream) {
        wxPrintf("Error: Unable to open connection\n");
        return false;
    }
    
    wxFileOutputStream outputStream(tempPath);
    if (!outputStream.IsOk()) {
        wxPrintf("Error: Unable to create output file\n");
        delete httpStream;
        return false;
    }
    
    char buffer[8192];
    size_t totalRead = 0;
    size_t lastRead = 0;
    
    while (!httpStream->Eof()) {
        httpStream->Read(buffer, sizeof(buffer));
        size_t read = httpStream->LastRead();
        
        if (read > 0) {
            outputStream.Write(buffer, read);
            totalRead += read;
            
            // Update progress every 64KB
            if (totalRead - lastRead > 65536) {
                lastRead = totalRead;
                
                wxString msg = wxString::Format("Downloaded: %.1f MB", totalRead / (1024.0 * 1024.0));
                if (!progressDialog.Update(totalRead / 1024, msg)) {
                    // User cancelled
                    delete httpStream;
                    outputStream.Close();
                    wxRemoveFile(tempPath);
                    return false;
                }
            }
        }
        
        wxYield(); // Keep UI responsive
    }
    
    delete httpStream;
    outputStream.Close();
    
    // Handle gzipped files
    if (url.EndsWith(".gz")) {
        wxPrintf("Extracting gzipped file...\n");
        if (!ExtractIfNeeded(tempPath)) {
            wxRemoveFile(tempPath);
            return false;
        }
    } else {
        // Rename to final location
        if (wxFileExists(outputPath)) {
            wxRemoveFile(outputPath);
        }
        if (!wxRenameFile(tempPath, outputPath)) {
            wxPrintf("Error: Unable to rename temporary file\n");
            return false;
        }
    }
    
    // Verify the database
    if (!VerifyDatabase(outputPath)) {
        wxPrintf("Warning: Downloaded file may be invalid\n");
    }
    
    return true;
}

bool GeoIPDownloadHelper::ExtractIfNeeded(const wxString& downloadedFile)
{
    wxString outputPath = m_configDir + wxFileName::GetPathSeparator() + "GeoLite2-Country.mmdb";
    
    wxFileInputStream inputStream(downloadedFile);
    if (!inputStream.IsOk()) {
        wxPrintf("Error: Unable to open downloaded file for extraction\n");
        return false;
    }
    
    wxZlibInputStream zlibStream(inputStream);
    wxFileOutputStream outputStream(outputPath);
    
    if (!outputStream.IsOk()) {
        wxPrintf("Error: Unable to create output file for extraction\n");
        return false;
    }
    
    char buffer[8192];
    while (!zlibStream.Eof()) {
        zlibStream.Read(buffer, sizeof(buffer));
        size_t read = zlibStream.LastRead();
        if (read > 0) {
            outputStream.Write(buffer, read);
        }
        wxYield();
    }
    
    outputStream.Close();
    wxRemoveFile(downloadedFile); // Remove temporary .gz file
    
    return true;
}

bool GeoIPDownloadHelper::VerifyDatabase(const wxString& databasePath)
{
    wxFile file(databasePath);
    if (!file.IsOpened()) {
        return false;
    }
    
    // Simple verification: check if file is reasonably sized
    wxFileOffset size = file.Length();
    file.Close();
    
    if (size < 1024 * 1024) { // Less than 1MB is suspicious
        wxPrintf("Warning: Database file seems too small (%lld bytes)\n", size);
        return false;
    }
    
    if (size > 100 * 1024 * 1024) { // More than 100MB is suspicious
        wxPrintf("Warning: Database file seems too large (%lld bytes)\n", size);
        return false;
    }
    
    wxPrintf("Database file size: %.1f MB\n", size / (1024.0 * 1024.0));
    return true;
}

void GeoIPDownloadHelper::ShowHelp()
{
    wxPrintf("GeoIP Database Download Helper for aMule\n");
    wxPrintf("Usage: geoip_download_helper [options]\n\n");
    wxPrintf("Options:\n");
    wxPrintf("  -h, --help           Show this help message\n");
    wxPrintf("  -d, --dir DIR        Specify aMule config directory\n");
    wxPrintf("                       Default: ~/.aMule\n\n");
    wxPrintf("This tool helps download the required GeoIP database for aMule\n");
    wxPrintf("to enable country flag display for clients and servers.\n");
}

wxIMPLEMENT_APP(GeoIPDownloadHelper);