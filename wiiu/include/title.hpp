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

#ifndef TITLE_HPP
#define TITLE_HPP

#include "SDLHelper.hpp"
#include "account.hpp"
#include "configuration.hpp"
#include "io.hpp"
#include <algorithm>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <malloc.h>
#include <mxml.h>

#include <coreinit/mcp.h>
#include <nn/act.h>
#include <nn/pdm.h>

typedef uint32_t AccountUid;

class Title {
public:
    void init(uint64_t titleid, AccountUid userID, const std::string& name, const std::string& author);
    void initvWii(const std::string&name, const std::string& author);
    ~Title(void){};

    bool isvWii(void);
    std::string author(void);
    std::pair<std::string, std::string> displayName(void);
    SDL_Texture* icon(void);
    uint64_t id(void);
    std::string name(void);
    std::string path(void);
    std::string sourcePath(void);
    void sourcePath(std::string spath);
    uint32_t playTimeMinutes(void);
    std::string playTime(void);
    void playTimeMinutes(uint32_t playTimeMinutes);
    uint32_t lastPlayedTimestamp(void);
    void lastPlayedTimestamp(uint32_t lastPlayedTimestamp);
    std::string fullPath(size_t index);
    void refreshDirectories(void);
    uint64_t saveId();
    void saveId(uint64_t id);
    std::vector<std::string> saves(void);
    uint8_t saveDataType(void);
    AccountUid userId(void);
    std::string userName(void);

private:
    bool misvWii;
    uint64_t mId;
    bool mCommonSave;
    AccountUid mUserId;
    std::string mUserName;
    std::string mName;
    std::string mSafeName;
    std::string mAuthor;
    std::string mPath;
    std::string mSourcePath;
    std::vector<std::string> mSaves;
    std::vector<std::string> mFullSavePaths;
    std::pair<std::string, std::string> mDisplayName;
    uint32_t mPlayTimeMinutes;
    uint32_t mLastPlayedTimestamp;
};

void getTitle(Title& dst, AccountUid uid, size_t i);
size_t getTitleCount(AccountUid uid);
void loadTitles();
void sortTitles(void);
void rotateSortMode(void);
void refreshDirectories(uint64_t id);
bool favorite(AccountUid uid, int i);
void freeIcons(void);
SDL_Texture* smallIcon(AccountUid uid, size_t i);
std::unordered_map<std::string, std::string> getCompleteTitleList(void);

#endif