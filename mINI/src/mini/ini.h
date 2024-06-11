/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Danijel Durakovic
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

///////////////////////////////////////////////////////////////////////////////
//
//  /mINI/ v0.9.15
//  An INI file reader and writer for the modern age.
//
///////////////////////////////////////////////////////////////////////////////
//
//  A tiny utility library for manipulating INI files with a straightforward
//  API and a minimal footprint. It conforms to the (somewhat) standard INI
//  format - sections and keys are case insensitive and all leading and
//  trailing whitespace is ignored. Comments are lines that begin with a
//  semicolon. Trailing comments are allowed on section lines.
//
//  Files are read on demand, upon which data is kept in memory and the file
//  is closed. This utility supports lazy writing, which only writes changes
//  and updates to a file and preserves custom formatting and comments. A lazy
//  write invoked by a write() call will read the output file, find what
//  changes have been made and update the file accordingly. If you only need to
//  generate files, use generate() instead. Section and key order is preserved
//  on read, write and insert.
//
///////////////////////////////////////////////////////////////////////////////
//
//  /* BASIC USAGE EXAMPLE: */
//
//  /* read from file */
//  mINI::INIFile file("myfile.ini");
//  mINI::INIStructure ini;
//  file.read(ini);
//
//  /* read value; gets a reference to actual value in the structure.
//     if key or section don't exist, a new empty value will be created */
//  std::string& value = ini["section"]["key"];
//
//  /* read value safely; gets a copy of value in the structure.
//     does not alter the structure */
//  std::string value = ini.get("section").get("key");
//
//  /* set or update values */
//  ini["section"]["key"] = "value";
//
//  /* set multiple values */
//  ini["section2"].set({
//      {"key1", "value1"},
//      {"key2", "value2"}
//  });
//
//  /* write updates back to file, preserving comments and formatting */
//  file.write(ini);
//
//  /* or generate a file (overwrites the original) */
//  file.generate(ini);
//
///////////////////////////////////////////////////////////////////////////////
//
//  Long live the INI file!!!
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MINI_INI_H_
#define MINI_INI_H_

#include <filesystem>
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <unordered_map>
#include <vector>
#include <memory>
#include <fstream>
#include <sys/stat.h>
#include <cctype>

namespace mINI
{
    namespace INIStringUtil
    {
        const char* const whitespaceDelimiters = " \t\n\r\f\v";
        inline void trim(std::string& str)
        {
            str.erase(str.find_last_not_of(whitespaceDelimiters) + 1);
            str.erase(0, str.find_first_not_of(whitespaceDelimiters));
        }
#ifndef MINI_CASE_SENSITIVE
        inline void toLower(std::string& str)
        {
            std::transform(str.begin(), str.end(), str.begin(), [](const char c) {
                return static_cast<char>(std::tolower(c));
            });
        }
#endif
        inline void replace(std::string& str, std::string const& a, std::string const& b)
        {
            if (!a.empty())
            {
                std::size_t pos = 0;
                while ((pos = str.find(a, pos)) != std::string::npos)
                {
                    str.replace(pos, a.size(), b);
                    pos += b.size();
                }
            }
        }
#ifdef _WIN32
        const char* const endl = "\r\n";
#else
        const char* const endl = "\n";
#endif
    }

    template<typename T>
    class INIMap
    {
    private:
        using T_DataIndexMap = std::unordered_map<std::string, std::size_t>;
        using T_DataItem = std::tuple<std::string, T, std::string>;
        using T_DataContainer = std::vector<T_DataItem>;
        using T_MultiArgs = typename std::vector<std::tuple<std::string, T, std::string>>;

        T_DataIndexMap dataIndexMap;
        T_DataContainer data;

        inline std::size_t setEmpty(std::string& key)
        {
            std::size_t index = data.size();
            dataIndexMap[key] = index;
            data.emplace_back(key, T(), std::string());
            return index;
        }

    public:
        using const_iterator = typename T_DataContainer::const_iterator;

        INIMap() { }

        INIMap(INIMap const& other) : dataIndexMap(other.dataIndexMap), data(other.data)
        {
        }

        INIMap& operator=(INIMap const& other)
        {
            std::size_t data_size = other.data.size();
            for (std::size_t i = 0; i < data_size; ++i)
            {
                auto const& key = std::get<0>(other.data[i]);
                auto const& obj = std::get<1>(other.data[i]);
                auto const& com = std::get<2>(other.data[i]);
                data.emplace_back(key, obj, com);
            }
            dataIndexMap = T_DataIndexMap(other.dataIndexMap);
            return *this;
        }

        T& operator[](std::string key)
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            auto it = dataIndexMap.find(key);
            bool hasIt = (it != dataIndexMap.end());
            std::size_t index = (hasIt) ? it->second : setEmpty(key);
            return std::get<1>(data[index]);
        }
        T get(std::string key) const
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            auto it = dataIndexMap.find(key);
            if (it == dataIndexMap.end())
            {
                return T();
            }
            return T(std::get<1>(data[it->second]));
        }
        void setComment(std::string key, std::string comment)
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            auto it = dataIndexMap.find(key);
            if (it != dataIndexMap.end())
                std::get<2>(data[it->second]) = comment;
        }
        T getComment(std::string key) const
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            auto it = dataIndexMap.find(key);
            if (it == dataIndexMap.end())
            {
                return T();
            }
            return T(std::get<2>(data[it->second]));
        }
        bool has(std::string key) const
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            return (dataIndexMap.count(key) == 1);
        }
        size_t count(std::string key) const
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            return std::count_if(data.begin(), data.end(), [&key](const auto& i) { return std::get<0>(i) == key; });
        }
        void set(std::string key, T obj, std::string com = {}, bool bAllowMultiple = false)
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            auto it = dataIndexMap.find(key);
            if (!bAllowMultiple && it != dataIndexMap.end())
            {
                std::get<1>(data[it->second]) = obj;
                std::get<2>(data[it->second]) = com;
            }
            else
            {
                dataIndexMap[key] = data.size();
                data.emplace_back(key, obj, com);
            }
        }
        void set(T_MultiArgs const& multiArgs)
        {
            for (auto const& it : multiArgs)
            {
                auto const& key = std::get<0>(it);
                auto const& obj = std::get<1>(it);
                auto const& com = std::get<2>(it);
                set(key, obj, com);
            }
        }
        bool remove(std::string key)
        {
            INIStringUtil::trim(key);
#ifndef MINI_CASE_SENSITIVE
            INIStringUtil::toLower(key);
#endif
            auto it = dataIndexMap.find(key);
            if (it != dataIndexMap.end())
            {
                std::size_t index = it->second;
                data.erase(data.begin() + index);
                dataIndexMap.erase(it);
                for (auto& it2 : dataIndexMap)
                {
                    auto& vi = it2.second;
                    if (vi > index)
                    {
                        vi--;
                    }
                }
                return true;
            }
            return false;
        }
        void clear()
        {
            data.clear();
            dataIndexMap.clear();
        }
        std::size_t size() const
        {
            return data.size();
        }
        const_iterator begin() const { return data.begin(); }
        const_iterator end() const { return data.end(); }
    };

    using INIStructure = INIMap<INIMap<std::string>>;

    namespace INIParser
    {
        using T_ParseValues = std::tuple<std::string, std::string, std::string>;

        enum class PDataType : char
        {
            PDATA_NONE,
            PDATA_COMMENT,
            PDATA_SECTION,
            PDATA_KEYVALUE_COMMENT,
            PDATA_UNKNOWN
        };

        inline size_t getCommentAt(std::string_view line)
        {
            auto commentAt = line.find_last_of(';');
            if (commentAt == std::string::npos)
            {
                commentAt = line.rfind("//");
                if (commentAt == std::string::npos)
                    commentAt = line.find_last_of('#');
            }

            if (commentAt != std::string::npos && line.at(commentAt - 1) != ' ')
                commentAt = std::string::npos;

            return commentAt;
        }

        inline PDataType parseLine(std::string line, T_ParseValues& parseData)
        {
            std::get<0>(parseData).clear();
            std::get<1>(parseData).clear();
            std::get<2>(parseData).clear();
            INIStringUtil::trim(line);
            if (line.empty())
            {
                return PDataType::PDATA_NONE;
            }
            char firstCharacter = line[0];
            char secondCharacter = line[1];
            if (firstCharacter == '#' || firstCharacter == ';' || (firstCharacter == '/' && secondCharacter == '/'))
            {
                std::get<2>(parseData) = line;
                return PDataType::PDATA_COMMENT;
            }
            if (firstCharacter == '[')
            {
                auto commentAt = getCommentAt(line);
                if (commentAt != std::string::npos)
                {
                    line = line.substr(0, commentAt);
                }
                auto closingBracketAt = line.find_last_of(']');
                if (closingBracketAt != std::string::npos)
                {
                    auto section = line.substr(1, closingBracketAt - 1);
                    INIStringUtil::trim(section);
                    std::get<0>(parseData) = section;
                    return PDataType::PDATA_SECTION;
                }
            }
            auto lineNorm = line;
            INIStringUtil::replace(lineNorm, "\\=", "  ");
            auto equalsAt = lineNorm.find_first_of('=');
            if (equalsAt != std::string::npos)
            {
                auto key = line.substr(0, equalsAt);
                INIStringUtil::trim(key);
                INIStringUtil::replace(key, "\\=", "=");
                auto value = line.substr(equalsAt + 1);
                auto comment = std::string();
                auto commentAt = getCommentAt(line);
                if (commentAt != std::string::npos && commentAt > equalsAt) {
                    value = line.substr(0, commentAt);
                    value = value.substr(equalsAt + 1);
                    comment = line.substr(commentAt);
                    auto ident = value;
                    while (ident.back() == ' ')
                    {
                        ident.pop_back();
                        comment.insert(0, " ");
                    }
                }
                INIStringUtil::trim(value);
                std::get<0>(parseData) = key;
                std::get<1>(parseData) = value;
                std::get<2>(parseData) = comment;
                return PDataType::PDATA_KEYVALUE_COMMENT;
            }
            return PDataType::PDATA_UNKNOWN;
        }
    }

    class INIReader
    {
    public:
        using T_LineData = std::vector<std::string>;
        using T_LineDataPtr = std::shared_ptr<T_LineData>;

        bool isBOM = false;

    private:
        std::ifstream fileReadStream;
        T_LineDataPtr lineData;

        T_LineData readFile()
        {
            fileReadStream.seekg(0, std::ios::end);
            const std::size_t fileSize = static_cast<std::size_t>(fileReadStream.tellg());
            fileReadStream.seekg(0, std::ios::beg);
            if (fileSize >= 3) {
                const char header[3] = {
                    static_cast<char>(fileReadStream.get()),
                    static_cast<char>(fileReadStream.get()),
                    static_cast<char>(fileReadStream.get())
                };
                isBOM = (
                    header[0] == static_cast<char>(0xEF) &&
                    header[1] == static_cast<char>(0xBB) &&
                    header[2] == static_cast<char>(0xBF)
                );
            }
            else {
                isBOM = false;
            }
            std::string fileContents;
            fileContents.resize(fileSize);
            fileReadStream.seekg(isBOM ? 3 : 0, std::ios::beg);
            fileReadStream.read(&fileContents[0], fileSize);
            fileReadStream.close();
            T_LineData output;
            if (fileSize == 0)
            {
                return output;
            }
            std::string buffer;
            buffer.reserve(50);
            for (std::size_t i = 0; i < fileSize; ++i)
            {
                char& c = fileContents[i];
                if (c == '\n')
                {
                    output.emplace_back(buffer);
                    buffer.clear();
                    continue;
                }
                if (c != '\0' && c != '\r')
                {
                    buffer += c;
                }
            }
            output.emplace_back(buffer);
            return output;
        }

    public:
        INIReader(std::filesystem::path const& filename, bool keepLineData = false)
        {
            fileReadStream.open(filename, std::ios::in | std::ios::binary);
            if (keepLineData)
            {
                lineData = std::make_shared<T_LineData>();
            }
        }
        ~INIReader() { }

        bool operator>>(INIStructure& data)
        {
            if (!fileReadStream.is_open())
            {
                return false;
            }
            T_LineData fileLines = readFile();
            std::string section;
            bool inSection = false;
            INIParser::T_ParseValues parseData;
            for (auto const& line : fileLines)
            {
                auto parseResult = INIParser::parseLine(line, parseData);
                if (parseResult == INIParser::PDataType::PDATA_SECTION)
                {
                    inSection = true;
                    data[section = std::get<0>(parseData)];
                }
                else if (inSection && (parseResult == INIParser::PDataType::PDATA_KEYVALUE_COMMENT || 
                parseResult == INIParser::PDataType::PDATA_NONE || parseResult == INIParser::PDataType::PDATA_COMMENT))
                {
                    auto const& key = std::get<0>(parseData);
                    auto const& value = std::get<1>(parseData);
                    auto const& comment = std::get<2>(parseData);
                    data[section].set(key, value, comment, true);
                }
                if (lineData && parseResult != INIParser::PDataType::PDATA_UNKNOWN)
                {
                    if (parseResult == INIParser::PDataType::PDATA_KEYVALUE_COMMENT && !inSection)
                    {
                        continue;
                    }
                    lineData->emplace_back(line);
                }
            }
            return true;
        }
        T_LineDataPtr getLines()
        {
            return lineData;
        }
    };

    class INIGenerator
    {
    private:
        std::ofstream fileWriteStream;

    public:
        bool prettyPrint = false;

        INIGenerator(std::filesystem::path const& filename)
        {
            fileWriteStream.open(filename, std::ios::out | std::ios::binary);
        }
        ~INIGenerator() { }

        bool operator<<(INIStructure const& data)
        {
            if (!fileWriteStream.is_open())
            {
                return false;
            }
            if (!data.size())
            {
                return true;
            }
            auto it = data.begin();
            for (;;)
            {
                auto const& section = std::get<0>(*it);
                auto const& collection = std::get<1>(*it);
                fileWriteStream
                    << "["
                    << section
                    << "]";
                if (collection.size())
                {
                    fileWriteStream << INIStringUtil::endl;
                    auto it2 = collection.begin();
                    for (;;)
                    {
                        auto key = std::get<0>(*it2);
                        INIStringUtil::replace(key, "=", "\\=");
                        auto value = std::get<1>(*it2);
                        INIStringUtil::trim(value);
                        auto comment = std::get<2>(*it2);
                        fileWriteStream
                            << key
                            << ((key.length() || value.length()) ? ((prettyPrint) ? " = " : "=") : "")
                            << value
                            << comment;
                        if (++it2 == collection.end())
                        {
                            break;
                        }
                        fileWriteStream << INIStringUtil::endl;
                    }
                }
                if (++it == data.end())
                {
                    break;
                }
                fileWriteStream << INIStringUtil::endl;
                if (prettyPrint)
                {
                    fileWriteStream << INIStringUtil::endl;
                }
            }
            return true;
        }
    };

    class INIWriter
    {
    private:
        using T_LineData = std::vector<std::string>;
        using T_LineDataPtr = std::shared_ptr<T_LineData>;

        std::filesystem::path filename;

        T_LineData getLazyOutput(T_LineDataPtr const& lineData, INIStructure& data, INIStructure& original)
        {
            T_LineData output;
            INIParser::T_ParseValues parseData;
            std::string sectionCurrent;
            bool parsingSection = false;
            bool continueToNextSection = false;
            bool discardNextEmpty = false;
            bool writeNewKeys = false;
            std::size_t lastKeyLine = 0;
            for (auto line = lineData->begin(); line != lineData->end(); ++line)
            {
                if (!writeNewKeys)
                {
                    auto parseResult = INIParser::parseLine(*line, parseData);
                    if (parseResult == INIParser::PDataType::PDATA_SECTION)
                    {
                        if (parsingSection)
                        {
                            writeNewKeys = true;
                            parsingSection = false;
                            --line;
                            continue;
                        }
                        sectionCurrent = std::get<0>(parseData);
                        if (data.has(sectionCurrent))
                        {
                            parsingSection = true;
                            continueToNextSection = false;
                            discardNextEmpty = false;
                            output.emplace_back(*line);
                            lastKeyLine = output.size();
                        }
                        else
                        {
                            continueToNextSection = true;
                            discardNextEmpty = true;
                            continue;
                        }
                    }
                    else if (parseResult == INIParser::PDataType::PDATA_KEYVALUE_COMMENT)
                    {
                        if (continueToNextSection)
                        {
                            continue;
                        }
                        if (data.has(sectionCurrent))
                        {
                            auto& collection = data[sectionCurrent];
                            auto const& key = std::get<0>(parseData);
                            auto const& value = std::get<1>(parseData);
                            auto const& comment = std::get<2>(parseData);
                            if (collection.has(key))
                            {
                                auto outputValue = collection[key];
                                auto outputComment = collection.getComment(key);
                                if (value == outputValue || collection.count(key) > 1)
                                {
                                    output.emplace_back(*line);
                                }
                                else
                                {
                                    INIStringUtil::trim(outputValue);
                                    auto lineNorm = *line;
                                    INIStringUtil::replace(lineNorm, "\\=", "  ");
                                    auto equalsAt = lineNorm.find_first_of('=');
                                    auto valueAt = lineNorm.find_first_not_of(
                                        INIStringUtil::whitespaceDelimiters,
                                        equalsAt + 1
                                    );
                                    std::string outputLine = line->substr(0, valueAt);
                                    if (prettyPrint && equalsAt + 1 == valueAt)
                                    {
                                        outputLine += " ";
                                    }
                                    outputLine += outputValue;

                                    if (!outputComment.empty() && outputComment.front() == ' ')
                                    {
                                        std::ptrdiff_t i = outputValue.size() - value.size();
                                        if (i < 0)
                                        {
                                            while (i < 0) {
                                                outputComment.insert(0, " ");
                                                i++;
                                            }
                                        }
                                        else if (i > 0)
                                        {
                                            auto pos = outputComment.find_first_not_of(' ');
                                            while (i > 0 && pos >= size_t(i)) {
                                                if (outputComment.front() == ' ')
                                                    outputComment.erase(0, 1);
                                                i--;
                                            }
                                        }
                                    }
                                    outputLine += outputComment;
                                    output.emplace_back(outputLine);
                                }
                                lastKeyLine = output.size();
                            }
                        }
                    }
                    else
                    {
                        if (discardNextEmpty && line->empty())
                        {
                            discardNextEmpty = false;
                        }
                        else if (parseResult != INIParser::PDataType::PDATA_UNKNOWN)
                        {
                            output.emplace_back(*line);
                        }
                    }
                }
                if (writeNewKeys || std::next(line) == lineData->end())
                {
                    T_LineData linesToAdd;
                    if (data.has(sectionCurrent) && original.has(sectionCurrent))
                    {
                        auto const& collection = data[sectionCurrent];
                        auto const& collectionOriginal = original[sectionCurrent];
                        for (auto const& it : collection)
                        {
                            auto key = std::get<0>(it);
                            if (collectionOriginal.has(key))
                            {
                                continue;
                            }
                            auto value = std::get<1>(it);
                            INIStringUtil::replace(key, "=", "\\=");
                            INIStringUtil::trim(value);
                            linesToAdd.emplace_back(
                                key + ((prettyPrint) ? " = " : "=") + value
                            );
                        }
                    }
                    if (!linesToAdd.empty())
                    {
                        output.insert(
                            output.begin() + lastKeyLine,
                            linesToAdd.begin(),
                            linesToAdd.end()
                        );
                    }
                    if (writeNewKeys)
                    {
                        writeNewKeys = false;
                        --line;
                    }
                }
            }
            for (auto const& it : data)
            {
                auto const& section = std::get<0>(it);
                if (original.has(section))
                {
                    continue;
                }
                if (prettyPrint && output.size() > 0 && !output.back().empty())
                {
                    output.emplace_back();
                }
                output.emplace_back("[" + section + "]");
                auto const& collection = std::get<1>(it);
                for (auto const& it2 : collection)
                {
                    auto key = std::get<0>(it2);
                    auto value = std::get<1>(it2);
                    INIStringUtil::replace(key, "=", "\\=");
                    INIStringUtil::trim(value);
                    output.emplace_back(
                        key + ((prettyPrint) ? " = " : "=") + value
                    );
                }
            }
            return output;
        }

    public:
        bool prettyPrint = false;

        INIWriter(std::filesystem::path const& filename)
        : filename(filename)
        {
        }
        ~INIWriter() { }

        bool operator<<(INIStructure& data)
        {
            std::error_code ec;
            if (!std::filesystem::exists(filename, ec))
            {
                INIGenerator generator(filename);
                generator.prettyPrint = prettyPrint;
                return generator << data;
            }
            INIStructure originalData;
            T_LineDataPtr lineData;
            bool readSuccess = false;
            bool fileIsBOM = false;
            {
                INIReader reader(filename, true);
                if ((readSuccess = reader >> originalData))
                {
                    lineData = reader.getLines();
                    fileIsBOM = reader.isBOM;
                }
            }
            if (!readSuccess)
            {
                return false;
            }
            T_LineData output = getLazyOutput(lineData, data, originalData);
            std::ofstream fileWriteStream(filename, std::ios::out | std::ios::binary);
            if (fileWriteStream.is_open())
            {
                if (fileIsBOM) {
                    const char utf8_BOM[3] = {
                        static_cast<char>(0xEF),
                        static_cast<char>(0xBB),
                        static_cast<char>(0xBF)
                    };
                    fileWriteStream.write(utf8_BOM, 3);
                }
                if (output.size())
                {
                    auto line = output.begin();
                    for (;;)
                    {
                        fileWriteStream << *line;
                        if (++line == output.end())
                        {
                            break;
                        }
                        fileWriteStream << INIStringUtil::endl;
                    }
                }
                return true;
            }
            return false;
        }
    };

    class INIFile
    {
    private:
        std::filesystem::path filename;

    public:
        INIFile(std::filesystem::path const& filename)
        : filename(filename)
        { }

        ~INIFile() { }

        bool read(INIStructure& data) const
        {
            if (data.size())
            {
                data.clear();
            }
            if (filename.empty())
            {
                return false;
            }
            INIReader reader(filename);
            return reader >> data;
        }
        bool generate(INIStructure const& data, bool pretty = false) const
        {
            if (filename.empty())
            {
                return false;
            }
            INIGenerator generator(filename);
            generator.prettyPrint = pretty;
            return generator << data;
        }
        bool write(INIStructure& data, bool pretty = false) const
        {
            if (filename.empty())
            {
                return false;
            }
            INIWriter writer(filename);
            writer.prettyPrint = pretty;
            return writer << data;
        }
    };
}

#endif // MINI_INI_H_
