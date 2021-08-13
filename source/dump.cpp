#include "dump.hpp"
#include <tesla.hpp>

//enum class Result {
//    Success = 0,
//    NotImplemented = 1,
//    InvalidArgument = 2,
//    InProgress = 3,
//    NoAsyncOperation = 4,
//    InvalidAsyncOperation = 5,
//    NotPermitted = 6,
//    NotInitialized = 7,
//};

static char pathBuffer[FS_MAX_PATH] = { 0 };
Result rc;

/**
 * @brief Dumps dream town
 *
 * @param progress : pointer to a progressbar progress
 * @param *status : pointer to a progressbar status string (const char*)
 * @param *logelm : pointer to a Log Element
 */
void Dumper(u8* progress, const char** status, tsl::elm::Log** logelm) {
	*progress = 69;
	*status = "optaining pointers...";
	//[[[main+3DFE1D8]+10]+130]+60
	u64 mainAddr = util::FollowPointerMain(0x3DFE1D8, 0x10, 0x130, 0xFFFFFFFFFFFFFFFF) + 0x60;
	if (mainAddr == 0x60) {
		*status = "Error: mainAddr";
		*progress = 0;
		return;
	}
	//[[[[main+3DFE1D8]+10]+140]+08]
	u64 playerAddr = util::FollowPointerMain(0x3DFE1D8, 0x10, 0x140, 0x08, 0xFFFFFFFFFFFFFFFF);
	if (playerAddr == 0x00) {
		*status = "Error: playerAddr";
		*progress = 0;
		return;
	}

	*progress = 20;
	*status = "pointers optained";

	//max amount of players in a dream town
	std::vector<bool> players(0x8, false);

	*status = "checking players...";
	//check existing players
	for (u8 i = 0; i < 8; i++) {
		u64 offset = i * GSavePlayerVillagerAccountSize;
		u128 AccountUID = 0;
		dmntchtReadCheatProcessMemory(mainAddr + GSavePlayerVillagerAccountOffset + offset, &AccountUID, 0x10);
		if (AccountUID != 0) players[i] = true;
	}
	*status = "players checked";
	//time for fun
	fsdevMountSdmc();
	FsFileSystem fsSdmc;
	rc = fsOpenSdCardFileSystem(&fsSdmc);
	if (R_FAILED(rc)) {
		*progress = 0;
		*status = "Error: opening SD Card";
		fsdevUnmountDevice("sdmc");
		return;
	}
	FsFile main;
	FsFile personal;

	TimeCalendarTime dumpdreamtime = util::getDreamTime(mainAddr);
	char dreamtime[128];
	const char* date_format = "%02d.%02d.%04d @ %02d-%02d";
	sprintf(dreamtime, date_format, dumpdreamtime.day, dumpdreamtime.month, dumpdreamtime.year, dumpdreamtime.hour, dumpdreamtime.minute);
	
	(*logelm)->addLine("Dream Time: " + std::string(dreamtime));

	(*logelm)->addLine("DA-" + util::getDreamAddrString(mainAddr));

	std::string newdumppath = "/config/luna/dump/[DA-" + util::getDreamAddrString(mainAddr) + "]";
	std::string strislandname = util::getIslandNameASCII(playerAddr);
	if (!strislandname.empty()) newdumppath += " " + strislandname;

	*status = "starting dump...";
	//make dir on SD
	if (access(newdumppath.c_str(), F_OK) == -1) {
		mkdir(newdumppath.c_str(), 0777);
	}
	newdumppath += "/" + std::string(dreamtime);
	mkdir(newdumppath.c_str(), 0777);
	for (u8 i = 0; i < 8; i++) {
		fs::addPathFilter("/config/luna/template/Villager" + std::to_string(i));
	}
	//copy template to new directory recursively
	fs::copyDirToDir(&fsSdmc, "/config/luna/template/", newdumppath + "/", logelm);
	fs::freePathFilters();
	(*logelm)->addLine("copied template.");
	*progress = 40;
	*status = "finished copying template";
	size_t bufferSize = BUFF_SIZE;
	u8 *buffer = new u8[bufferSize];
	std::snprintf(pathBuffer, FS_MAX_PATH, std::string(newdumppath + "/main.dat").c_str());
	//opening main read
	rc = fsFsOpenFile(&fsSdmc, pathBuffer, FsOpenMode_Read, &main);
	if (R_FAILED(rc)) {
		*progress = 0;
		*status = "Error: opening main file";
		fsFileClose(&main);
		fsFsClose(&fsSdmc);
		fsdevUnmountDevice("sdmc");
		return;
	}
	//read AccountUID linkage (for Nintendo Switch Online)
	size_t AccountTableSize = 0x10 + (8 * 0x48); //0x250
	u8* SavePlayerVillagerAccountTableBuffer = new u8[AccountTableSize]; //0x250
	u64 AccountTableOffset = 0x10;
	u64 bytesread;
	fsFileRead(&main, SaveHeaderSize + GSavePlayerVillagerAccountOffset - AccountTableOffset, SavePlayerVillagerAccountTableBuffer, AccountTableSize, FsReadOption_None, &bytesread);

	//done reading main
	fsFileClose(&main);

	//opening main write
	rc = fsFsOpenFile(&fsSdmc, pathBuffer, FsOpenMode_Write, &main);
	if (R_FAILED(rc)) {
		*progress = 0;
		*status = "Error: opening main file";
		fsFileClose(&main);
		fsFsClose(&fsSdmc);
		fsdevUnmountDevice("sdmc");
		return;
	}

	*status = "writing main.dat...";
	for (u64 offset = 0; offset < mainSize; offset += bufferSize) {
		if (bufferSize > mainSize - offset)
			bufferSize = mainSize - offset;

		*progress = 40 + (20 * (offset / mainSize));

		dmntchtReadCheatProcessMemory(mainAddr + offset, buffer, bufferSize);
		rc = fsFileWrite(&main, SaveHeaderSize + offset, buffer, bufferSize, FsWriteOption_Flush);
#if DEBUG
		char out[sizeof(Result) + 13];
		snprintf(out, sizeof(Result) + 13, "fsFileWrite: %u", rc);
		*status = (const char*)out;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
	}
	u16 IsDreamingBed = 0; //346
	u16 TapDreamEnable = 1; //354
	u16 EnableMyDream = 0; //362
	u16 DreamUploadPlayerHaveCreatorID = 0; //364

	fsFileWrite(&main, SaveHeaderSize + EventFlagOffset + (346 * 2), &IsDreamingBed, sizeof(u16), FsWriteOption_Flush);
	fsFileWrite(&main, SaveHeaderSize + EventFlagOffset + (354 * 2), &TapDreamEnable, sizeof(u16), FsWriteOption_Flush);
	fsFileWrite(&main, SaveHeaderSize + EventFlagOffset + (362 * 2), &EnableMyDream, sizeof(u16), FsWriteOption_Flush);
	fsFileWrite(&main, SaveHeaderSize + EventFlagOffset + (364 * 2), &DreamUploadPlayerHaveCreatorID, sizeof(u16), FsWriteOption_Flush);

	//write AccountUID linkage (for Nintendo Switch Online)
	fsFileWrite(&main, SaveHeaderSize + GSavePlayerVillagerAccountOffset - AccountTableOffset, SavePlayerVillagerAccountTableBuffer, 0x10, FsWriteOption_Flush);
	for (u8 i = 0; i < 8; i++) {
		if (players[i]) {
			fsFileWrite(&main, SaveHeaderSize + GSavePlayerVillagerAccountOffset + (i * 0x48), SavePlayerVillagerAccountTableBuffer + 0x10 + (i * 0x48), 0x10, FsWriteOption_Flush);
		}
	}

	//remove DreamInfo in dumped file
	u8 DreamInfoBuffer[DreamInfoSize] = { 0 };
	fsFileWrite(&main, SaveHeaderSize + DreamIDOffset, DreamInfoBuffer, DreamInfoSize, FsWriteOption_Flush);


	(*logelm)->addLine("applied fixes to main.");

	*progress = 60;
	*status = "wrote main.dat";

	//done writing main
	fsFileClose(&main);

	FsFile landname;
	//clear our path buffer or bad things will happen
	memset(pathBuffer, 0, FS_MAX_PATH);
	std::snprintf(pathBuffer, FS_MAX_PATH, std::string(newdumppath + "/landname.dat").c_str());
	u16 islandname[0xB];
	memcpy(islandname, util::getIslandName(playerAddr).name, sizeof(islandname));
	//in case user doesn't submit a valid landname.dat file or the file at all
	fsFsDeleteFile(&fsSdmc, pathBuffer);
	fsFsCreateFile(&fsSdmc, pathBuffer, 0xB * sizeof(u16), 0);
	fsFsOpenFile(&fsSdmc, pathBuffer, FsOpenMode_Write, &landname);
	fsFileWrite(&landname, 0, &islandname, 0x16, FsWriteOption_Flush);
	fsFileClose(&landname);


	std::string Villager0 = newdumppath + "/Villager0/";

	u8 playercount = 0;

	for (u8 j = 0; j < 8; j++)
		if (players[j]) playercount++;

	u8 percentageperplayer = 40 / playercount;

	for (u8 i = 0; i < 8; i++) {
		//if this player (i) doesn't exist, return
		if (!players[i]) continue;

		std::string player = "/Villager" + std::to_string(i) + "/";
#if DEBUG
		(*logelm)->addLine(player);

#endif
		std::string currentplayer = newdumppath + player;
#if DEBUG
		(*logelm)->addLine(currentplayer);
#endif
		//disabling this due to issues with NetPlayInfo on per-player basis
		//copy the original villager for the existing villagers
		/*
		if (i != 0) {
			mkdir(currentplayer.c_str(), 0777);
			fs::copyDirToDir(&fsSdmc, Villager0, currentplayer, logelm);
			(*logelm)->addLine("finished copying player template Villager" + std::to_string(i) + ".");
		}
		*/
		//new implementation 
		mkdir(currentplayer.c_str(), 0777);
		fs::copyDirToDir(&fsSdmc, "/config/luna/template/" + player, currentplayer, logelm);
		(*logelm)->addLine("finished copying player template Villager" + std::to_string(i) + ".");
		//reset size in-case it got changed in the latter for loop
		bufferSize = BUFF_SIZE;
		//clear our path buffer or bad things will happen
		memset(pathBuffer, 0, FS_MAX_PATH);
		//opening personal
		std::snprintf(pathBuffer, FS_MAX_PATH, std::string(currentplayer + "personal.dat").c_str());
		rc = fsFsOpenFile(&fsSdmc, pathBuffer, FsOpenMode_Write, &personal);
		if (R_FAILED(rc)) {
			*progress = 0;
			*status = "Error: opening player file";
			fsFileClose(&personal);
			fsFsClose(&fsSdmc);
			fsdevUnmountDevice("sdmc");
			return;
		}

		*status = "writing player...";
		for (u64 offset = 0; offset < playerSize; offset += bufferSize) {
			if (bufferSize > playerSize - offset)
				bufferSize = playerSize - offset;

			dmntchtReadCheatProcessMemory(playerAddr + offset + (i * playersOffset), buffer, bufferSize);
			fsFileWrite(&personal, SaveHeaderSize + offset, buffer, bufferSize, FsWriteOption_Flush);
		}
		(*logelm)->addLine("wrote player " + std::to_string(i + 1) + ".");

		*status = "applying fixes to player...";
		u8 houselvl = 0;
		u16 BuiltTownOffice = 0; //59
		u16 UpgradePocket30 = 0; //669
		u16 UpgradePocket40 = 0; //670
		u16 SellPocket40 = 0; //672
		u8 ReceivedItemPocket30; //9052
		u8 ReceivedItemPocket40; //11140
		u8 ExpandBaggage = 0;
		//using default sizes
		u32 storageSize = 80;
		u32 pocket1Size = 0;

		dmntchtReadCheatProcessMemory(mainAddr + houseLvlOffset + (i * houseSize), &houselvl, sizeof(u8));
		dmntchtReadCheatProcessMemory(mainAddr + EventFlagOffset + (59 * 2), &BuiltTownOffice, sizeof(u16));
		dmntchtReadCheatProcessMemory(playerAddr + (i * playersOffset) + EventFlagsPlayerOffset + (669 * 2), &UpgradePocket30, sizeof(u16));
		dmntchtReadCheatProcessMemory(playerAddr + (i * playersOffset) + EventFlagsPlayerOffset + (670 * 2), &UpgradePocket40, sizeof(u16));
		dmntchtReadCheatProcessMemory(playerAddr + (i * playersOffset) + PlayerOtherOffset + ItemCollectBitOffset + (0x235C / 8), &ReceivedItemPocket30, sizeof(u8));
		dmntchtReadCheatProcessMemory(playerAddr + (i * playersOffset) + PlayerOtherOffset + ItemCollectBitOffset + (0x2B84 / 8), &ReceivedItemPocket40, sizeof(u8));

		switch (houselvl) {
			//no need to change defaults
			case 1: break;
			case 2: storageSize = 120;
					break;
			case 3: storageSize = 240;
					break;
			case 4: storageSize = 320;
					break;
			case 5: storageSize = 400;
					break;
			case 6: storageSize = 800;
					break;
			case 7: storageSize = 1600;
					break;
			case 8: storageSize = 2400;
					break;
		}

		if (UpgradePocket30 == 1) {
			pocket1Size += 0xA;
			ExpandBaggage = 0x01;
			ReceivedItemPocket30 |= (1 << 4);
			SellPocket40 = 1;
			if (UpgradePocket40 == 1) {
				pocket1Size += 0xA;
				ReceivedItemPocket40 |= (1 << 4);
				ExpandBaggage = 0x02;
			}
			else if (((ReceivedItemPocket40 >> 4) & 1) == 1) ReceivedItemPocket40 ^= (1 << 4);
		}
		else if (((ReceivedItemPocket30 >> 4) & 1) == 1) ReceivedItemPocket30 ^= (1 << 4);

#if DEBUG
		(*logelm)->addLine("SellPocket40: " + std::to_string(SellPocket40));
		(*logelm)->addLine("UpgradePocket30: " + std::to_string(UpgradePocket30));
		(*logelm)->addLine("UpgradePocket40: " + std::to_string(UpgradePocket40));
		(*logelm)->addLine("ReceivedItemPocket30: " + std::to_string(((ReceivedItemPocket30 >> 4) & 1)));
		(*logelm)->addLine("ReceivedItemPocket40: " + std::to_string(((ReceivedItemPocket40 >> 4) & 1)));
		(*logelm)->addLine("ExpandBaggage: " + std::to_string(ExpandBaggage));
#endif

		fsFileWrite(&personal, StorageSizeOffset, &storageSize, sizeof(u32), FsWriteOption_Flush);
		fsFileWrite(&personal, Pocket1SizeOffset, &pocket1Size, sizeof(u32), FsWriteOption_Flush);
		fsFileWrite(&personal, ExpandBaggageOffset, &ExpandBaggage, sizeof(u8), FsWriteOption_Flush);
		fsFileWrite(&personal, SaveHeaderSize + EventFlagsPlayerOffset + (672 * 2), &SellPocket40, sizeof(u16), FsWriteOption_Flush);
		fsFileWrite(&personal, SaveHeaderSize + PlayerOtherOffset + ItemCollectBitOffset + (0x235C / 8), &ReceivedItemPocket30, sizeof(u8), FsWriteOption_Flush);
		fsFileWrite(&personal, SaveHeaderSize + PlayerOtherOffset + ItemCollectBitOffset + (0x2B84 / 8), &ReceivedItemPocket40, sizeof(u8), FsWriteOption_Flush);

		fsFileClose(&personal);
		(*logelm)->addLine("applied fixes to player " + std::to_string(i + 1) + ".");
		*progress += percentageperplayer;
	}

	//dump succeeded

	*progress = 100;
	*status = "DONE!";
	fsFsClose(&fsSdmc);
	fsdevUnmountDevice("sdmc");
	return;
}