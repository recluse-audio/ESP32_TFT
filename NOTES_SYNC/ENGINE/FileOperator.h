/**
 * Made by Ryan Devens on 2026-02-25
 */

#pragma once
#include <string>
#include <vector>

/**
 * Abstract base class for file I/O operations on storage. Subclass per platform
 * (ESP32, Raylib desktop, etc.) to provide the appropriate implementation.
 */
class FileOperator
{
public:
    virtual ~FileOperator() = default;

    /**
     * Read the file at the given path and return its full contents as a string.
     * Returns an empty string if the file cannot be opened.
     */
    virtual std::string load(const std::string& path) = 0;

    /**
     * Write content to the file at the given path, overwriting any existing data.
     */
    virtual void writeToFile(const std::string& path, const std::string& content) = 0;

    /**
     * Append content to the file at the given path without overwriting existing data.
     */
    virtual void appendToFile(const std::string& path, const std::string& content) = 0;

    /**
     * Return the names of all entries (files and subdirectories) inside dirPath.
     * Returns an empty vector if the directory does not exist or cannot be opened.
     */
    virtual std::vector<std::string> listDirectory(const std::string& dirPath) = 0;

    /**
     * Write content to an absolute path that bypasses any data-root prefix.
     * Used for save files that live outside the game data root (e.g. FILE_GAME_SAVES).
     * Default implementation delegates to writeToFile, which is correct for platforms
     * whose writeToFile already handles absolute paths (e.g. Raylib on Windows).
     */
    virtual void writeAbsolute(const std::string& path, const std::string& content)
    {
        writeToFile(path, content);
    }

    /**
     * Read a file at an absolute path that bypasses any data-root prefix.
     * Default implementation delegates to load.
     */
    virtual std::string loadAbsolute(const std::string& path)
    {
        return load(path);
    }

};
