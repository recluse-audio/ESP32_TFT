/**
 * Made by Ryan Devens on 2026-02-25
 */

#pragma once
#include "FileOperator.h"
#include <SD.h>
#include <string>
#include <vector>

/**
 * ESP32 implementation of FileOperator.
 * Reads files from the SD card using the Arduino SD library.
 * Call setDataRoot() with the SD-card root folder (e.g. "/RD_GAME")
 * before using the file operator.
 */
class ESP32FileOperator : public FileOperator
{
public:
    void setDataRoot(const std::string& root) { mDataRoot = root; }

    std::string sdPath(const std::string& path) const
    {
        if (!path.empty() && path[0] == '/')
            return mDataRoot + path;
        return path;
    }

    std::string load(const std::string& path) override
    {
        File file = SD.open(sdPath(path).c_str());
        if (!file)
            return "";

        std::string content;
        while (file.available())
            content += static_cast<char>(file.read());

        file.close();
        return content;
    }

    void writeToFile(const std::string& path, const std::string& content) override
    {
        File file = SD.open(sdPath(path).c_str(), FILE_WRITE);
        if (file)
        {
            file.print(content.c_str());
            file.close();
        }
    }

    void appendToFile(const std::string& path, const std::string& content) override
    {
        File file = SD.open(sdPath(path).c_str(), FILE_APPEND);
        if (file)
        {
            file.print(content.c_str());
            file.close();
        }
    }

    void writeAbsolute(const std::string& path, const std::string& content) override
    {
        File file = SD.open(path.c_str(), FILE_WRITE);
        if (file)
        {
            file.print(content.c_str());
            file.close();
        }
    }

    std::string loadAbsolute(const std::string& path) override
    {
        File file = SD.open(path.c_str());
        if (!file) return "";
        std::string content;
        while (file.available())
            content += static_cast<char>(file.read());
        file.close();
        return content;
    }

    std::vector<std::string> listDirectory(const std::string& path) override
    {
        std::vector<std::string> entries;
        File dir = SD.open(sdPath(path).c_str());
        if (!dir) return entries;
        while (true)
        {
            File entry = dir.openNextFile();
            if (!entry) break;
            entries.push_back(std::string(entry.name()));
            entry.close();
        }
        dir.close();
        return entries;
    }

private:
    std::string mDataRoot;
};
