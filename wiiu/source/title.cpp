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

#include "title.hpp"
#include <stdint.h>

static std::unordered_map<AccountUid, std::vector<Title>> titles;
static std::unordered_map<uint64_t, SDL_Texture*> icons;

void freeIcons(void)
{
    for (auto& i : icons) {
        SDL_DestroyTexture(i.second);
    }
}

static void loadIcon(uint64_t id, uint8_t* icon, size_t iconsize)
{
    auto it = icons.find(id);
    if (it == icons.end()) {
        SDL_Texture* texture;
        SDLH_LoadImage(&texture, icon, iconsize, true);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
        icons.insert({id, texture});
    }
}

void Title::init(uint64_t id, AccountUid userID, const std::string& name, const std::string& author)
{
    mId           = id;
    mUserId       = userID;
    mUserName     = Account::username(userID);
    mAuthor       = author;
    mName         = name;
    mSafeName     = StringUtils::containsInvalidChar(name) ? StringUtils::format("0x%016llX", mId) : StringUtils::removeForbiddenCharacters(name);
    mPath         = "wiiu/Checkpoint/saves/" + StringUtils::format("0x%016llX", mId) + " " + mSafeName;

    std::string aname = StringUtils::removeAccents(mName);
    size_t pos        = aname.rfind(":");
    mDisplayName      = std::make_pair(name, "");
    if (pos != std::string::npos) {
        std::string name1 = aname.substr(0, pos);
        std::string name2 = aname.substr(pos + 1);
        StringUtils::trim(name1);
        StringUtils::trim(name2);
        mDisplayName.first  = name1;
        mDisplayName.second = name2;
    }
    else {
        // check for parenthesis
        size_t pos1 = aname.rfind("(");
        size_t pos2 = aname.rfind(")");
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            std::string name1 = aname.substr(0, pos1);
            std::string name2 = aname.substr(pos1 + 1, pos2 - 1 - pos1);
            StringUtils::trim(name1);
            StringUtils::trim(name2);
            mDisplayName.first  = name1;
            mDisplayName.second = name2;
        }
    }

    if (!io::directoryExists(mPath)) {
        io::createDirectory(mPath);
    }

    refreshDirectories();
}

void initvWii(const std::string&name, const std::string& author)
{

}

bool Title::isvWii(void)
{
    return misvWii;
}

uint64_t Title::id(void)
{
    return mId;
}

AccountUid Title::userId(void)
{
    return mUserId;
}

std::string Title::userName(void)
{
    return mUserName;
}

std::string Title::author(void)
{
    return mAuthor;
}

std::string Title::name(void)
{
    return mName;
}

std::pair<std::string, std::string> Title::displayName(void)
{
    return mDisplayName;
}

std::string Title::path(void)
{
    return mPath;
}

std::string Title::sourcePath(void)
{
    return mSourcePath;
}

void Title::sourcePath(std::string spath)
{
    mSourcePath = spath;
}

std::string Title::fullPath(size_t index)
{
    return mFullSavePaths.at(index);
}

std::vector<std::string> Title::saves()
{
    return mSaves;
}

SDL_Texture* Title::icon(void)
{
    auto it = icons.find(mId);
    return it != icons.end() ? it->second : NULL;
}

uint32_t Title::playTimeMinutes(void)
{
    return mPlayTimeMinutes;
}

std::string Title::playTime(void)
{
    return StringUtils::format("%d", mPlayTimeMinutes / 60) + ":" + StringUtils::format("%02d", mPlayTimeMinutes % 60) + " hours";
}

void Title::playTimeMinutes(uint32_t playTimeMinutes)
{
    mPlayTimeMinutes = playTimeMinutes;
}

uint32_t Title::lastPlayedTimestamp(void)
{
    return mLastPlayedTimestamp;
}

void Title::lastPlayedTimestamp(uint32_t lastPlayedTimestamp)
{
    mLastPlayedTimestamp = lastPlayedTimestamp;
}

void Title::refreshDirectories(void)
{
    mSaves.clear();
    mFullSavePaths.clear();

    Directory savelist(mPath);
    if (savelist.good()) {
        for (size_t i = 0, sz = savelist.size(); i < sz; i++) {
            if (savelist.folder(i)) {
                mSaves.push_back(savelist.entry(i));
                mFullSavePaths.push_back(mPath + "/" + savelist.entry(i));
            }
        }

        std::sort(mSaves.rbegin(), mSaves.rend());
        std::sort(mFullSavePaths.rbegin(), mFullSavePaths.rend());
        mSaves.insert(mSaves.begin(), "New...");
        mFullSavePaths.insert(mFullSavePaths.begin(), "New...");
    }
    else {
        Logger::getInstance().log(Logger::ERROR, "Couldn't retrieve the extdata directory list for the title " + name());
    }

    // save backups from configuration
    std::vector<std::string> additionalFolders = Configuration::getInstance().additionalSaveFolders(mId);
    for (std::vector<std::string>::const_iterator it = additionalFolders.begin(); it != additionalFolders.end(); ++it) {
        // we have other folders to parse
        Directory list(*it);
        if (list.good()) {
            for (size_t i = 0, sz = list.size(); i < sz; i++) {
                if (list.folder(i)) {
                    mSaves.push_back(list.entry(i));
                    mFullSavePaths.push_back(*it + "/" + list.entry(i));
                }
            }
        }
    }
}

void loadTitles()
{
    titles.clear();

    /* Wii U Titles */

    std::vector<std::string> paths { 
        "storage_mlc:/usr/save/00050000", // eShop title in internal storage
        "storage_mlc:/usr/save/00050002", // eShop title demo / Kiosk Interactive Demo in internal storage
        "storage_usb:/usr/save/00050000", // eShop title on USB
        "storage_usb:/usr/save/00050002", // eShop title demo / Kiosk Interactive Demo on USB
    }; 

    for (const auto& path : paths) {
        Directory dir(path);
        if (dir.good()) {
            for (uint32_t i = 0; i < dir.size(); i++) {
                if (dir.folder(i)) {
                    uint64_t titleId = 0;
                    std::string titleName;
                    std::string titlePublisher;

                    // Read metadata from meta.xml
                    std::string metaXmlPath = path + "/" + dir.entry(i) + "/meta/meta.xml";
                    FILE* metaXmlFile = fopen(metaXmlPath.c_str(), "rb");
                    if (metaXmlFile) {
                        // Get size
                        fseek(metaXmlFile, 0, SEEK_END);
                        size_t meta_size = ftell(metaXmlFile);
                        rewind(metaXmlFile);

                        char* metaXml = (char*) malloc(meta_size);
                        fread(metaXml, 1, meta_size, metaXmlFile);
                        fclose(metaXmlFile);

                        mxml_node_t *xt = NULL;
                        xt = mxmlLoadString(NULL, metaXml, MXML_OPAQUE_CALLBACK);
                        if(xt == NULL)
                            continue;
                        mxml_node_t *xm = mxmlGetFirstChild(xt);
                        if(xm == NULL)
                            continue;
                        mxml_node_t *xn = mxmlFindElement(xm, xt, "title_id", "type", "hexBinary", MXML_DESCEND);
                        if(xn) {
                            titleId = strtoull(mxmlGetOpaque(xn), NULL, 16);
                        }

                        if (Configuration::getInstance().filter(titleId)) {
                            continue;
                        }

                        xn = mxmlFindElement(xm, xt, "shortname_en", "type", "string", MXML_DESCEND);
                        if(xn) {
                            titleName.assign(mxmlGetOpaque(xn));
                        }

                        xn = mxmlFindElement(xm, xt, "publisher_en", "type", "string", MXML_DESCEND);
                        if(xn) {
                            titlePublisher.assign(mxmlGetOpaque(xn));
                        }
                        mxmlDelete(xt);
                    }
                    else {
                        Logger::getInstance().log(Logger::WARN, "No meta.xml for save " + path + dir.entry(i));
                        continue;
                    }

                    if (titleId == 0) {
                        continue;
                    }

                    std::string iconPath = path + "/" + dir.entry(i) + "/meta/iconTex.tga";
                    FILE* iconFile = fopen(iconPath.c_str(), "rb");
                    if (iconFile) {
                        fseek(iconFile, 0, SEEK_END);
                        size_t icon_size = ftell(iconFile);
                        rewind(iconFile);

                        uint8_t* icon = (uint8_t*) malloc(icon_size);
                        fread(icon, 1, icon_size, iconFile);
                        fclose(iconFile);

                        loadIcon(titleId, icon, icon_size);
                        free(icon);
                    }

                    Directory userdir(path + "/" + dir.entry(i) + "/user");
                    if (userdir.good()) {
                        for (uint32_t j = 0; j < userdir.size(); j++) {
                            if (userdir.folder(j)) {
                                AccountUid uid = 0;

                                // common save
                                if (std::strcmp(userdir.entry(j).c_str(), "common") == 0) {
                                    uid = COMMONSAVE_ID;
                                }
                                else {
                                    char* ptr;
                                    uid = (AccountUid) strtoul(userdir.entry(j).c_str(), &ptr, 16);
                                    if (!ptr) {
                                        continue;
                                    }
                                }

                                Title title;
                                title.init(titleId, uid, titleName, titlePublisher);
                                title.sourcePath(path + "/" + dir.entry(i) + "/user/" + userdir.entry(j));

                                if (uid != COMMONSAVE_ID) {
                                    nn::act::SlotNo accountSlot = accountIdToSlotNo(uid);
                                    if (accountSlot > 0) {
                                        // load play statistics
                                        nn::pdm::PlayStats stats;
                                        uint32_t res = nn::pdm::GetPlayStatsOfTitleId(&stats, accountSlot, titleId);
                                        title.playTimeMinutes(res == 0 ? stats.playtime : 0);
                                        title.lastPlayedTimestamp(res == 0 ? stats.last_time_played : 0);
                                    }
                                }

                                // check if the vector is already created
                                std::unordered_map<AccountUid, std::vector<Title>>::iterator it = titles.find(uid);
                                if (it != titles.end())  {
                                    // found
                                    it->second.push_back(title);
                                }
                                else  {
                                    // not found, insert into map
                                    std::vector<Title> v;
                                    v.push_back(title);
                                    titles.emplace(uid, v);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* TODO vWii titles */

    std::vector<std::string> vWiiPaths { 
        "slccmpt:/title/00010000", "slccmpt:/title/00010004", // Disc-based games
        "slccmpt:/title/00010001"                             // Downloaded channels
    }; 

    for (const auto& path : vWiiPaths) {
        Directory dir(path);
        if (dir.good()) {
            for (uint32_t i = 0; i < dir.size(); i++) {
                if (dir.folder(i)) {
                    
                }
            }
        }
    }

    sortTitles();
}

void sortTitles(void)
{
    for (auto& vect : titles) {
        std::sort(vect.second.begin(), vect.second.end(), [](Title& l, Title& r) {
            if (Configuration::getInstance().favorite(l.id()) != Configuration::getInstance().favorite(r.id())) {
                return Configuration::getInstance().favorite(l.id());
            }
            switch (g_sortMode) {
                case SORT_LAST_PLAYED:
                    return l.lastPlayedTimestamp() > r.lastPlayedTimestamp();
                case SORT_PLAY_TIME:
                    return l.playTimeMinutes() > r.playTimeMinutes();
                case SORT_ALPHA:
                default:
                    return l.name() < r.name();
            }
        });
    }
}

void rotateSortMode(void)
{
    g_sortMode = static_cast<sort_t>((g_sortMode + 1) % SORT_MODES_COUNT);
    sortTitles();
}

void getTitle(Title& dst, AccountUid uid, size_t i)
{
    std::unordered_map<AccountUid, std::vector<Title>>::iterator it = titles.find(uid);
    if (it != titles.end() && i < getTitleCount(uid)) {
        dst = it->second.at(i);
    }
}

size_t getTitleCount(AccountUid uid)
{
    std::unordered_map<AccountUid, std::vector<Title>>::iterator it = titles.find(uid);
    return it != titles.end() ? it->second.size() : 0;
}

bool favorite(AccountUid uid, int i)
{
    std::unordered_map<AccountUid, std::vector<Title>>::iterator it = titles.find(uid);
    return it != titles.end() ? Configuration::getInstance().favorite(it->second.at(i).id()) : false;
}

void refreshDirectories(uint64_t id)
{
    for (auto& pair : titles) {
        for (size_t i = 0; i < pair.second.size(); i++) {
            if (pair.second.at(i).id() == id) {
                pair.second.at(i).refreshDirectories();
            }
        }
    }
}

SDL_Texture* smallIcon(AccountUid uid, size_t i)
{
    std::unordered_map<AccountUid, std::vector<Title>>::iterator it = titles.find(uid);
    return it != titles.end() ? it->second.at(i).icon() : NULL;
}

std::unordered_map<std::string, std::string> getCompleteTitleList(void)
{
    std::unordered_map<std::string, std::string> map;
    for (const auto& pair : titles) {
        for (auto value : pair.second) {
            map.insert({StringUtils::format("0x%016llX", value.id()), value.name()});
        }
    }
    return map;
}
