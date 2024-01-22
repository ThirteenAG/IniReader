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
    mINI::INIStructure m_ini;

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

        mINI::INIFile file(m_szFileName);
        file.read(m_ini);
    }

    int ReadInteger(std::string_view szSection, std::string_view szKey, int iDefaultValue)
    {
        try
        {
            if (m_ini.has(szSection.data()))
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
            if (m_ini.has(szSection.data()))
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
            if (m_ini.has(szSection.data()))
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
            if (m_ini.has(szSection.data()))
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
        return szDefaultValue.data();
    }
    
    void WriteInteger(std::string_view szSection, std::string_view szKey, int iValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = std::to_string(iValue);
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
    
    void WriteFloat(std::string_view szSection, std::string_view szKey, float fltValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = std::to_string(fltValue);
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
    
    void WriteBoolean(std::string_view szSection, std::string_view szKey, bool bolValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = bolValue ? "True" : "False";
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
    
    void WriteString(std::string_view szSection, std::string_view szKey, std::string_view szValue, bool pretty = false)
    {
        try
        {
            mINI::INIFile file(m_szFileName);
            m_ini[szSection.data()][szKey.data()] = szValue.data();
            file.write(m_ini, pretty);
        }
        catch (...) {}
    }
};

#endif //INIREADER_H
