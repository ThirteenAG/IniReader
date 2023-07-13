#ifndef INIREADER_H
#define INIREADER_H
#include "ini_parser.hpp"
#include <string>
#include <string_view>
#include <Windows.h>
#include <filesystem>

/*
*  String comparision functions, with case sensitive option
*/

using std::strcmp;

inline int strcmp(const char* str1, const char* str2, bool csensitive)
{
    return (csensitive ? ::strcmp(str1, str2) : ::_stricmp(str1, str2));
}

inline int strcmp(const char* str1, const char* str2, size_t num, bool csensitive)
{
    return (csensitive ? ::strncmp(str1, str2, num) : ::_strnicmp(str1, str2, num));
}

inline int compare(const std::string& str1, const std::string& str2, bool case_sensitive)
{
    if (str1.length() == str2.length())
        return strcmp(str1.c_str(), str2.c_str(), case_sensitive);
    return (str1.length() < str2.length() ? -1 : 1);
}

inline int compare(const std::string& str1, const std::string& str2, size_t num, bool case_sensitive)
{
    if (str1.length() == str2.length())
        return strcmp(str1.c_str(), str2.c_str(), num, case_sensitive);
    return (str1.length() < str2.length() ? -1 : 1);
}

inline int compare(const char* str1, const char* str2, bool case_sensitive)
{
    return strcmp(str1, str2, case_sensitive);
}

inline int compare(const char* str1, const char* str2, size_t num, bool case_sensitive)
{
    return strcmp(str1, str2, num, case_sensitive);
}

inline bool starts_with(const char* str, const char* prefix, bool case_sensitive)
{
    while (*prefix)
    {
        bool equal;
        if (case_sensitive)
            equal = (*str++ == *prefix++);
        else
            equal = (::tolower(*str++) == ::tolower(*prefix++));

        if (!equal) return false;
    }
    return true;
}

inline bool ends_with(const char* str, const char* prefix, bool case_sensitive)
{
    auto str2 = &str[strlen(str) - 1];
    auto prefix2 = &prefix[strlen(prefix) - 1];

    while (prefix2 >= prefix)
    {
        bool equal;
        if (case_sensitive)
            equal = (*str2-- == *prefix2--);
        else
            equal = (::tolower(*str2--) == ::tolower(*prefix2--));

        if (!equal) return false;
    }
    return true;
}

class CIniReader
{
private:
    std::filesystem::path m_szFileName;

    // writes a key into the ini without losing other data around it
    // only writes the first found instance
    inline bool WriteIniString(std::string_view szSection, std::string_view szKey, std::string_view szValue, bool bKeepInlineData = false)
    {
        if (!std::filesystem::exists(m_szFileName))
        {
            std::ofstream inifile;
            inifile.open(m_szFileName);
            if (!inifile.is_open())
                return false;
            inifile.close();
        }

        std::ifstream ifile;
        ifile.open(m_szFileName, std::ios::binary);

        if (!ifile.is_open())
            return false;

        // read entire ini to buffer which will be used as a reference
        std::stringstream iniStream;
        iniStream << ifile.rdbuf();
        ifile.close();

        // write back the ini to the file with modified contents
        std::ofstream ofile;
        ofile.open(m_szFileName, std::ios::binary);
        if (!ofile.is_open())
            return false;

        std::string line;
        std::string write_line;
        bool bFirst = true;
        bool bInSection = false;
        bool bEnteredSectionOnce = false;
        bool bWrittenOnce = false;
        while (std::getline(iniStream, line))
        {
            if (line.empty())
                continue;

            // trim any newline chars
            line.erase(std::find_if(line.rbegin(), line.rend(), std::not_fn(std::function<int(int)>(::isspace))).base(), line.end());

            write_line = line;
            if (!bWrittenOnce)
            {
                if (line.front() == '[' && line.back() == ']')
                {
                    bFirst = false;
                    if (line.find(szSection) != std::string::npos)
                    {
                        bInSection = true;
                        bEnteredSectionOnce = true;
                    }
                    else
                        bInSection = false;
                }

                if (bInSection)
                {
                    if (line.find(szKey) != std::string::npos)
                    {
                        size_t pos;
                        std::string clean_line = line;

                        // Find comment and remove anything after it from the line
                        if ((pos = clean_line.find_first_of(';')) != clean_line.npos)
                            clean_line.erase(pos);

                        if ((pos = clean_line.rfind("//")) != clean_line.npos)
                            clean_line.erase(pos);

                        clean_line.erase(std::find_if(clean_line.rbegin(), clean_line.rend(), std::not_fn(std::function<int(int)>(::isspace))).base(), clean_line.end());

                        write_line = clean_line.substr(clean_line.find(szKey), clean_line.rfind('='));
                        write_line += "=";
                        write_line += szValue;

                        if (bKeepInlineData)
                        {
                            bool bAddSpace = false;
                            std::string comment = line.substr(line.find('=') + 1);
                            // after the equals sign we may have a space, so check for that and trim it
                            comment.erase(comment.begin(), std::find_if(comment.begin(), comment.end(), std::not_fn(std::function<int(int)>(::isspace))));
                            // check if there even is a comment
                            if ((comment.find(';') != std::string::npos) || (comment.find("//") != std::string::npos))
                            {
                                // search for the first instance of a space and move it there
                                pos = 0;
                                for (char i : comment)
                                {
                                    if (isspace(i))
                                        break;
                                    pos++;
                                }
                                if (pos)
                                {
                                    // move back if we overshoot
                                    size_t backpos = 0;
                                    if ((backpos = comment.rfind(';', pos)) != std::string::npos)
                                    {
                                        pos = backpos;
                                        bAddSpace = true;
                                    }
                                    else if ((backpos = comment.rfind("//", pos)) != std::string::npos)
                                    {
                                        pos = backpos;
                                        bAddSpace = true;
                                    }

                                    comment = comment.substr(pos);
                                }
                                else if (comment.find(';') != std::string::npos)
                                {
                                    comment = comment.substr(comment.find(';'));
                                    bAddSpace = true;
                                }
                                else if (comment.find("//") != std::string::npos)
                                {
                                    comment = comment.substr(comment.find("//"));
                                    bAddSpace = true;
                                }

                                if (bAddSpace)
                                    write_line += ' ';
                                write_line += comment;
                            }
                        }

                        bWrittenOnce = true;
                    }
                }
                else if (bEnteredSectionOnce)
                {
                    ofile << szKey << "=" << szValue << "\r\n";
                    bWrittenOnce = true;
                }
            }

            ofile << write_line << "\r\n";
            ofile.flush();
        }

        if (!bWrittenOnce)
        {
            if (!bEnteredSectionOnce)
            {
                if (!bFirst)
                    ofile << "\r\n";
                ofile << '[' << szSection << ']' << "\r\n";
            }
            ofile << szKey << "=" << szValue << "\r\n";
        }

        ofile.close();

        return true;
    }

public:
    linb::ini data;

    CIniReader()
    {
        SetIniPath("");
    }

    CIniReader(std::filesystem::path szFileName)
    {
        SetIniPath(szFileName);
    }

    CIniReader(std::stringstream& ini_mem)
    {
        data.load_file(ini_mem);
    }

    bool operator==(CIniReader& ir)
    {
        if (data.size() != ir.data.size())
            return false;

        for (auto& section : data)
        {
            for (auto& key : data.at(section.first))
            {
                if (key.second != ir.data.at(section.first)[key.first])
                    return false;
            }
        }
        return true;
    }

    bool operator!=(CIniReader& ir)
    {
        return !(*this == ir);
    }

    bool CompareBySections(CIniReader& ir)
    {
        if (data.size() != ir.data.size())
            return false;

        for (auto& section : data)
        {
            if (ir.data.find(section.first) == ir.data.end())
                return false;

            if (section.second.size() != ir.data.find(section.first)->second.size())
                return false;

            if (section.first != ir.data.find(section.first)->first)
                return false;
        }
        return true;
    }

    bool CompareByValues(CIniReader& ir)
    {
        return *this == ir;
    }

    const std::filesystem::path& GetIniPath()
    {
        return m_szFileName;
    }

    void SetIniPath()
    {
        SetIniPath("");
    }

    void SetIniPath(std::filesystem::path szFileName)
    {
        //char buffer[MAX_PATH];
        WCHAR buffer[MAX_PATH];
        HMODULE hm = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&ends_with, &hm);
        GetModuleFileNameW(hm, buffer, ARRAYSIZE(buffer));

        std::filesystem::path modulePath(buffer);
        std::u8string strFileName = szFileName.u8string();

        if (strFileName.find(':') != std::u8string::npos)
        {
            m_szFileName = szFileName;
        }
        else if (strFileName.length() == 0)
        {
            m_szFileName = modulePath.replace_extension(".ini");
        }
        else
        {
            m_szFileName = modulePath.u8string().substr(0, modulePath.u8string().rfind('\\') + 1) + strFileName.data();
        }

        data.load_file(m_szFileName);
    }

    int ReadInteger(std::string_view szSection, std::string_view szKey, int iDefaultValue)
    {
        auto str = data.get(szSection.data(), szKey.data(), std::to_string(iDefaultValue));
        return std::stoi(str, nullptr, starts_with(str.c_str(), "0x", false) ? 16 : 10);
    }

    float ReadFloat(std::string_view szSection, std::string_view szKey, float fltDefaultValue)
    {
        return (float)atof(data.get(szSection.data(), szKey.data(), std::to_string(fltDefaultValue)).c_str());
    }

    bool ReadBoolean(std::string_view szSection, std::string_view szKey, bool bolDefaultValue)
    {
        auto val = data.get(szSection.data(), szKey.data(), "");
        if (!val.empty())
        {
            if (val.size() == 1)
                return val.front() != '0';
            else
                return compare(val, "false", false);
        }
        return bolDefaultValue;
    }

    std::string ReadString(std::string_view szSection, std::string_view szKey, std::string_view szDefaultValue)
    {
        auto s = data.get(szSection.data(), szKey.data(), szDefaultValue.data());

        if (!s.empty())
        {
            if (s.at(0) == '\"' || s.at(0) == '\'')
                s.erase(0, 1);

            if (s.at(s.size() - 1) == '\"' || s.at(s.size() - 1) == '\'')
                s.erase(s.size() - 1);
        }

        return s;
    }

    void WriteInteger(std::string_view szSection, std::string_view szKey, int iValue, bool useparser = false)
    {
        if (useparser)
        {
            data.set(szSection.data(), szKey.data(), std::to_string(iValue));
            data.write_file(m_szFileName);
        }
        else
        {
            char szValue[255];
            _snprintf_s(szValue, 255, "%s%d", " ", iValue);
            WriteIniString(szSection, szKey, szValue, true);
        }
    }

    void WriteFloat(std::string_view szSection, std::string_view szKey, float fltValue, bool useparser = false)
    {
        if (useparser)
        {
            data.set(szSection.data(), szKey.data(), std::to_string(fltValue));
            data.write_file(m_szFileName);
        }
        else
        {
            char szValue[255];
            _snprintf_s(szValue, 255, "%s%f", " ", fltValue);
            WriteIniString(szSection, szKey, szValue, true);
        }
    }

    void WriteBoolean(std::string_view szSection, std::string_view szKey, bool bolValue, bool useparser = false)
    {
        if (useparser)
        {
            data.set(szSection.data(), szKey.data(), std::to_string(bolValue));
            data.write_file(m_szFileName);
        }
        else
        {
            char szValue[255];
            _snprintf_s(szValue, 255, "%s%s", " ", bolValue ? "True" : "False");
            WriteIniString(szSection, szKey, szValue, true);
        }
    }

    void WriteString(std::string_view szSection, std::string_view szKey, std::string_view szValue, bool useparser = false)
    {
        if (useparser)
        {
            data.set(szSection.data(), szKey.data(), szValue.data());
            data.write_file(m_szFileName);
        }
        else
        {
            WriteIniString(szSection, szKey, szValue);
        }
    }
};

#endif //INIREADER_H
