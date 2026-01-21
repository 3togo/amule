//
// MaxMindDBDatabase.h - MaxMind database implementation for GeoIP
//
#ifndef MAXMIND_DB_DATABASE_H
#define MAXMIND_DB_DATABASE_H

#include "IGeoIPDatabase.h"
#include <wx/string.h>

#ifdef ENABLE_MAXMINDDB
#include <maxminddb.h>
#endif

class MaxMindDBDatabase : public IGeoIPDatabase
{
public:
    MaxMindDBDatabase();
    ~MaxMindDBDatabase() override;
    
    bool Open(const wxString& databasePath) override;
    void Close() override;
    bool IsOpen() const;  // Not overriding base interface, it's internal
    bool IsValid() const override;
    DatabaseType GetType() const override;
    wxString GetFormatName() const override;
    wxString GetVersion() const override;
    wxString GetDescription() const override;
    
    wxString GetCountryCode(const wxString& ip) override;
    wxString GetCountryName(const wxString& ip) override;
    
private:
    bool LookupCountry(const wxString& ip, wxString& country_code, wxString& country_name) const;
#ifdef ENABLE_MAXMINDDB
    MMDB_s m_mmdb;        // MaxMind DB handle
#endif
    bool m_isOpen;
    wxString m_dbPath;    // Store path for metadata
};

#endif // MAXMIND_DB_DATABASE_H