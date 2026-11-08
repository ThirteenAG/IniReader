#ifndef INIREADER_H
#define INIREADER_H

#define MINI_CASE_SENSITIVE
#include "mINI\src\mini\ini.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>

class CIniReader
{
private:
    std::filesystem::path m_szFileName;
    std::filesystem::path m_szLoadedFileName;
    mINI::INIStructure m_ini;

    void WriteValue(std::string_view szSection, std::string_view szKey, std::string szValue, bool pretty)
    {
        try
        {
            mINI::INIFile file(m_szFileName);

            // Pick up the current file contents before changing it, so a failed read at construction
            // (file locked, not synced yet, ...) or external edits can't make the write drop sections.
            // Skipped when saving to a different file via SetNewIniPathForSave.
            if (m_szFileName == m_szLoadedFileName)
            {
                std::error_code ec;
                bool fileExists = std::filesystem::exists(m_szFileName, ec);
                if (ec)
                    return;

                if (fileExists)
                {
                    mINI::INIStructure current;
                    if (!file.read(current))
                        return;
                    m_ini = current;
                }
            }

            m_ini[szSection.data()][szKey.data()] = szValue;
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }

public:
    CIniReader()
    {
        SetIniPath();
    }

    CIniReader(std::filesystem::path szFileName)
    {
        SetIniPath(szFileName);
    }

    bool operator==(CIniReader& ir)
    {
        auto& a = m_ini;
        auto& b = ir.m_ini;
        for (auto const& it : a)
        {
            auto const& section = std::get<0>(it);
            auto const& collection = std::get<1>(it);
            if (collection.size() != b[section].size()) {
                return false;
            }
            for (auto const& it2 : collection)
            {
                auto const& key = std::get<0>(it2);
                auto const& value = std::get<1>(it2);
                if (value != b[section][key]) {
                    return false;
                }
            }
        }
        return a.size() == b.size();
    }

    bool operator!=(CIniReader& ir)
    {
        return !(*this == ir);
    }

    bool CompareBySections(CIniReader& ir)
    {
        std::vector<std::string> sections1;
        std::vector<std::string> sections2;

        for (auto const& it : m_ini)
            sections1.emplace_back(std::get<0>(it));

        for (auto const& it : ir.m_ini)
            sections2.emplace_back(std::get<0>(it));

        return std::equal(sections1.begin(), sections1.end(), sections2.begin(), sections2.end());
    }

    bool CompareByValues(CIniReader& ir)
    {
        return *this == ir;
    }

    const std::filesystem::path& GetIniPath()
    {
        return m_szFileName;
    }

    void SetNewIniPathForSave(std::filesystem::path szFileName)
    {
        m_szFileName = szFileName;
    }

    void SetIniPath()
    {
        SetIniPath("");
    }

    void SetIniPath(std::filesystem::path szFileName)
    {
        static const auto lpModuleName = 1;
        WCHAR buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&lpModuleName, &hm);
        GetModuleFileNameW(hm, buffer, ARRAYSIZE(buffer));
        std::filesystem::path modulePath(buffer);
        // The module's name as it is on disk: a loader may load it through a path in another case, and the ini made
        // next to it should match the plugin's name
        WIN32_FIND_DATAW found{};
        if (HANDLE search = FindFirstFileExW(modulePath.c_str(), FindExInfoBasic, &found, FindExSearchNameMatch, nullptr, 0); search != INVALID_HANDLE_VALUE)
        {
            FindClose(search);
            modulePath.replace_filename(found.cFileName);
        }

        if (szFileName.is_absolute())
        {
            m_szFileName = szFileName;
        }
        else if (szFileName.empty())
        {
            m_szFileName = modulePath.replace_extension(".ini");
        }
        else
        {
            m_szFileName = modulePath.parent_path() / szFileName;
        }

        m_szLoadedFileName = m_szFileName;
        mINI::INIFile file(m_szFileName);
        file.read(m_ini);
    }

    int ReadInteger(std::string_view szSection, std::string_view szKey, int iDefaultValue)
    {
        try
        {
            if (m_ini.size() && m_ini.has(szSection.data()))
            {
                auto& collection = m_ini[szSection.data()];
                if (collection.has(szKey.data()))
                {
                    auto& value = collection[szKey.data()];
                    return std::stoi(value, nullptr, (value.starts_with("0x") || value.starts_with("0X")) ? 16 : 10);
                }
            }
        }
        catch (...) {}
        return iDefaultValue;
    }

    float ReadFloat(std::string_view szSection, std::string_view szKey, float fltDefaultValue)
    {
        try
        {
            if (m_ini.size() && m_ini.has(szSection.data()))
            {
                auto& collection = m_ini[szSection.data()];
                if (collection.has(szKey.data()))
                {
                    auto& value = collection[szKey.data()];
                    return static_cast<float>(std::atof(value.data()));
                }
            }
        }
        catch (...) {}
        return fltDefaultValue;
    }
    
    bool ReadBoolean(std::string_view szSection, std::string_view szKey, bool bolDefaultValue)
    {
        try
        {
            if (m_ini.size() && m_ini.has(szSection.data()))
            {
                auto& collection = m_ini[szSection.data()];
                if (collection.has(szKey.data()))
                {
                    auto value = collection[szKey.data()];
                    if (value.size() == 1)
                        return value != "0";
                    else {
                        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
                        if (value == "false")
                            return false;
                        else if (value == "true")
                            return true;
                    }
                }
            }
        }
        catch (...) {}
        return bolDefaultValue;
    }
    
    std::string ReadString(std::string_view szSection, std::string_view szKey, std::string_view szDefaultValue)
    {
        try
        {
            if (m_ini.size() && m_ini.has(szSection.data()))
            {
                auto& collection = m_ini[szSection.data()];
                if (collection.has(szKey.data()))
                {
                    auto value = collection[szKey.data()];
                    if (!value.empty())
                    {
                        if (value.at(0) == '\"' || value.at(0) == '\'')
                            value.erase(0, 1);
                        if (value.at(value.size() - 1) == '\"' || value.at(value.size() - 1) == '\'')
                            value.erase(value.size() - 1);
                    }
                    return value;
                }
            }
        }
        catch (...) {}
        return std::string(szDefaultValue);
    }
    
    void WriteInteger(std::string_view szSection, std::string_view szKey, int iValue, bool pretty = false)
    {
        WriteValue(szSection, szKey, std::to_string(iValue), pretty);
    }
    
    void WriteFloat(std::string_view szSection, std::string_view szKey, float fltValue, bool pretty = false)
    {
        WriteValue(szSection, szKey, std::to_string(fltValue), pretty);
    }
    
    void WriteBoolean(std::string_view szSection, std::string_view szKey, bool bolValue, bool pretty = false)
    {
        WriteValue(szSection, szKey, bolValue ? "True" : "False", pretty);
    }
    
    void WriteString(std::string_view szSection, std::string_view szKey, std::string_view szValue, bool pretty = false)
    {
        WriteValue(szSection, szKey, std::string(szValue), pretty);
    }
};

#endif //INIREADER_H
