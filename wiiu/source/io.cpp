/*
 *   This file is part of Checkpoint
 *   Copyright (C) 2017-2019 Bernardo Giordano, FlagBrew
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#include "io.hpp"

bool io::fileExists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void io::copyFile(const std::string& srcPath, const std::string& dstPath, int mode)
{
    g_isTransferringFile = true;

    FILE* src = fopen(srcPath.c_str(), "rb");
    if (src == NULL) {
        Logger::getInstance().log(Logger::ERROR, "Failed to open source file " + srcPath + " during copy with errno %d. Skipping...", errno);
        return;
    }
    FILE* dst = fopen(dstPath.c_str(), "wb");
    if (dst == NULL) {
        Logger::getInstance().log(Logger::ERROR, "Failed to open destination file " + dstPath + " during copy with errno %d. Skipping...", errno);
        fclose(src);
        return;
    }

    char *buffer[3];
    for (int i = 0; i < 3; i++) {
        buffer[i] = (char *) aligned_alloc(0x40, IO_MAX_FILE_BUFFER);
        if (buffer[i] == nullptr) {
            fclose(src);
            fclose(dst);
            for (i--; i >= 0; i--)
                free(buffer[i]);

            return;
        }
    }

    setvbuf(src, buffer[0], _IOFBF, IO_MAX_FILE_BUFFER);
    setvbuf(dst, buffer[1], _IOFBF, IO_MAX_FILE_BUFFER);

    struct stat st;
    if (stat(srcPath.c_str(), &st) < 0)
        return;
    uint64_t sz = st.st_size;
    uint64_t offset = 0;
    g_currentFile   = basename(srcPath.c_str());

    while (offset < sz) {
        uint32_t count = fread(buffer[2], 1, IO_MAX_FILE_BUFFER, src);
        fwrite(buffer[2], 1, count, dst);
        offset += count;

        // avoid freezing the UI
        // this will be made less horrible next time...
        g_screen->draw();
        SDLH_Render();
    }

    for (int i = 0; i < 3; i++)
        free(buffer[i]);

    fclose(src);
    fclose(dst);

    if (mode)
        chmod(dstPath.c_str(), mode);

    g_isTransferringFile = false;
}

int32_t io::copyDirectory(const std::string& srcPath, const std::string& dstPath, int mode)
{
    int32_t res = 0;
    bool quit  = false;
    Directory items(srcPath);

    if (!items.good()) {
        return items.error();
    }

    for (size_t i = 0, sz = items.size(); i < sz && !quit; i++) {
        std::string newsrc = srcPath + items.entry(i);
        std::string newdst = dstPath + items.entry(i);

        if (items.folder(i)) {
            res = io::createDirectory(newdst, mode);
            if (res == 0) {
                newsrc += "/";
                newdst += "/";
                res = io::copyDirectory(newsrc, newdst, mode);
            }
            else {
                quit = true;
            }
        }
        else {
            io::copyFile(newsrc, newdst, mode);
        }
    }

    return 0;
}

int32_t io::createDirectory(const std::string& path, int mode)
{
    mkdir(path.c_str(), mode ? mode : 777);
    if (mode) chmod(path.c_str(), mode);
    return 0;
}

bool io::directoryExists(const std::string& path)
{
    struct stat sb;
    return (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

int32_t io::deleteFolderRecursively(const std::string& path)
{
    Directory dir(path);
    if (!dir.good()) {
        return dir.error();
    }

    for (size_t i = 0, sz = dir.size(); i < sz; i++) {
        if (dir.folder(i)) {
            std::string newpath = path + "/" + dir.entry(i) + "/";
            deleteFolderRecursively(newpath);
            newpath = path + dir.entry(i);
            rmdir(newpath.c_str());
        }
        else {
            std::string newpath = path + dir.entry(i);
            std::remove(newpath.c_str());
        }
    }

    rmdir(path.c_str());
    return 0;
}

std::tuple<bool, int32_t, std::string> io::backup(size_t index, AccountUid uid, size_t cellIndex)
{
    const bool isNewFolder                     = cellIndex == 0;
    int32_t res                                = 0;
    std::tuple<bool, int32_t, std::string> ret = std::make_tuple(false, -1, "");

    Title title;
    getTitle(title, uid, index);

    Logger::getInstance().log(Logger::INFO, "Started backup of %s. Title id: 0x%llX; User id: 0x%016lX.", title.name().c_str(), title.id(),
        title.userId());

    std::string suggestion = DateTime::dateTimeStr() + " " +
                             (StringUtils::containsInvalidChar(Account::username(title.userId()))
                                     ? ""
                                     : StringUtils::removeNotAscii(StringUtils::removeAccents(Account::username(title.userId()))));
    std::string customPath;

    if (MS::multipleSelectionEnabled()) {
        customPath = isNewFolder ? suggestion : "";
    }
    else {
        if (isNewFolder) {
            if (KeyboardManager::get().isInitialized()) {
                std::pair<bool, std::string> keyboardResponse = KeyboardManager::get().keyboard(suggestion);
                if (keyboardResponse.first) {
                    customPath = StringUtils::removeForbiddenCharacters(keyboardResponse.second);
                }
                else {
                    Logger::getInstance().log(Logger::INFO, "Copy operation aborted by the user through the system keyboard.");
                    return std::make_tuple(false, 0, "Operation aborted by the user.");
                }
            }
            else {
                customPath = suggestion;
            }
        }
        else {
            customPath = "";
        }
    }

    std::string dstPath;
    if (!isNewFolder) {
        // we're overriding an existing folder
        dstPath = title.fullPath(cellIndex);
    }
    else {
        dstPath = title.path() + "/" + customPath;
    }

    if (!isNewFolder || io::directoryExists(dstPath)) {
        int rc = io::deleteFolderRecursively((dstPath + "/").c_str());
        if (rc != 0) {
            Logger::getInstance().log(Logger::ERROR, "Failed to recursively delete directory " + dstPath + " with result %d.", rc);
            return std::make_tuple(false, (int32_t)rc, "Failed to delete the existing backup\ndirectory recursively.");
        }
    }

    io::createDirectory(dstPath);
    res = io::copyDirectory(title.sourcePath() + "/", dstPath + "/");
    if (res != 0) {
        io::deleteFolderRecursively((dstPath + "/").c_str());
        Logger::getInstance().log(Logger::ERROR, "Failed to copy directory " + dstPath + " with result 0x%08lX. Skipping...", res);
        return std::make_tuple(false, res, "Failed to backup save.");
    }

    refreshDirectories(title.id());

    if (!MS::multipleSelectionEnabled()) {
        blinkLed(4);
        ret = std::make_tuple(true, 0, "Progress correctly saved to disk.");
    }

    if (!KeyboardManager::get().isInitialized()) {
        return std::make_tuple(true, -1,
            "Progress correctly saved to disk.\nSystem keyboard applet was not\naccessible. The suggested destination\nfolder was used instead.");
    }

    ret = std::make_tuple(true, 0, "Progress correctly saved to disk.");
    Logger::getInstance().log(Logger::INFO, "Backup succeeded.");
    return ret;
}

std::tuple<bool, int32_t, std::string> io::restore(size_t index, AccountUid uid, size_t cellIndex, const std::string& nameFromCell)
{
    int32_t res                                = 0;
    std::tuple<bool, int32_t, std::string> ret = std::make_tuple(false, -1, "");
    Title title;
    getTitle(title, uid, index);

    Logger::getInstance().log(Logger::INFO, "Started restore of %s. Title id: 0x%016llX; User id: 0x%lX.", title.name().c_str(), title.id(), title.userId());

    std::string srcPath = title.fullPath(cellIndex) + "/";
    std::string dstPath = title.sourcePath() + "/";

    Directory dir(dstPath);
    if (!dir.good()) {
        Logger::getInstance().log(Logger::ERROR, "Failed to recursively delete directory " + dstPath + " with result 0x%08lX.", dir.error());
        return std::make_tuple(false, res, "Failed to delete save.");
    }
    else {
        for (size_t i = 0; i < dir.size(); i++)  {
            if (dir.folder(i)) {
                std::string newpath = dstPath + "/" + dir.entry(i) + "/";
                deleteFolderRecursively(newpath);
                newpath = dstPath + dir.entry(i);
                rmdir(newpath.c_str());
            }
            else {
                std::string newpath = dstPath + dir.entry(i);
                std::remove(newpath.c_str());
            }
        }
    }

    res = io::copyDirectory(srcPath, dstPath, 0x666); // 0x666 is required for saves to work properly
    if (res != 0) {
        Logger::getInstance().log(Logger::ERROR, "Failed to copy directory " + srcPath + " to " + dstPath + " with result 0x%08lX. Skipping...", res);
        return std::make_tuple(false, res, "Failed to restore save.");
    }

    res = flushVolume("/vol/storage_mlc01");
    if (res != 0) {
        Logger::getInstance().log(Logger::WARN, "Failed to flush mlc");
    }

    res = flushVolume("/vol/storage_usb01");
    if (res != 0) {
        Logger::getInstance().log(Logger::WARN, "Failed to flush usb");
    } 

    blinkLed(4);
    ret = std::make_tuple(true, 0, nameFromCell + "\nhas been restored successfully.");
    Logger::getInstance().log(Logger::INFO, "Restore succeeded.");
    return ret;
}