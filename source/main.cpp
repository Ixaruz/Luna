#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
#include <util.h>
#include "dump.hpp"
#include "luna.h"

static char ret[100];
static bool isRunning = false;
static bool checkedFiles = false;

typedef struct {
    /* 0x00 */ u32 Major;
    /* 0x04 */ u32 Minor;
    /* 0x08 */ u16 Unk1;
    /* 0x0A */ u16 HeaderRevision;
    /* 0x0C */ u16 Unk2;
    /* 0x0E */ u16 SaveRevision;
} FileHeaderInfo;

//seperate thread that runs after createUI or something
#if !DEBUG_UI
std::thread dump;
#endif

class GuiDump : public tsl::Gui {
public:
    //ctor
    GuiDump(tsl::Overlay* ovlmaster) { this->mamaoverlay = ovlmaster; }

    //dtor
    ~GuiDump() {
#if !DEBUG_UI
        dump.join();
#endif
        this->mamaoverlay->close();
        isRunning = false;
    }

    // Called once when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element *createUI() override {
        auto frame = new tsl::elm::OverlayFrame("Luna", STRING_VERSION);

        this->m_progressBar = new tsl::elm::ProgressBar();
        this->m_Log = new tsl::elm::Log(20);
        auto list = new tsl::elm::List();

        list->addItem(this->m_progressBar);
        list->addItem(this->m_Log);
#if !DEBUG_UI
        //assign dump thread our Dumper function
        dump = std::thread([this]() { Dumper(&this->progress, &this->status, &this->m_Log); });
#endif
        isRunning = true;

        frame->setContent(list);

        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {

        if (isRunning) {
            this->tem++;
            this->m_progressBar->setProgress(this->progress);
            this->m_progressBar->setStatus(this->status);
            //should change 4 times a second
            if (this->tem % 15 == 0) {
                this->m_progressBar->Spin();
                this->tem = 0;
            }
        }
    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
        return false;   // Return true here to signal the inputs have been consumed
    }

private:
    tsl::Overlay *mamaoverlay = nullptr;
    tsl::elm::ProgressBar *m_progressBar = nullptr;
    u8 progress = 0;
    u8 tem = 0;
    const char *status = "preparing...";
    tsl::elm::Log *m_Log = nullptr;
};

class SelectionGui : public tsl::Gui {
public:
    SelectionGui(tsl::Overlay* ovlmaster) { this->papaoverlay = ovlmaster; }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("Luna", STRING_VERSION);

        // A list that can contain sub elements and handles scrolling
        auto list = new tsl::elm::List();

        // List Items
        list->addItem(new tsl::elm::CategoryHeader("Dump Options"));

        auto* clickableListItem = new tsl::elm::ListItem("Dump Dream Island", "...");
        clickableListItem->setClickListener([this](u64 keys) {
            if (keys & KEY_A) {
                tsl::changeTo<GuiDump>(this->papaoverlay);
                return true;
            }

            return false;
            });

        list->addItem(clickableListItem);

        // Add the list to the frame for it to be drawn
        frame->setContent(list);

        // Return the frame to have it become the top level element of this Gui
        return frame;
    }

    // Called once every frame to update values
    virtual void update() override {
    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
        return false;   // Return true here to signal the inputs have been consumed
    }

private:
    tsl::Overlay *papaoverlay = nullptr;
};

class GuiError : public tsl::Gui {
public:
    GuiError(tsl::elm::Element* elm) { this->m_elm = elm; }

    virtual tsl::elm::Element *createUI() override {

        tsl::elm::OverlayFrame *rootFrame = new tsl::elm::OverlayFrame("Luna", STRING_VERSION);

        rootFrame->setContent(this->m_elm);

        return rootFrame;
    }
private:
    tsl::elm::Element *m_elm;
};

enum class CheckResult {
    Success,
    WrongBID,
    WrongTID,
    NoDream,
    NoTemplate,
    WrongRevision,
    NotEnoughPlayers,
};

struct Check {
    CheckResult check_result;
    tsl::elm::Element* elm;
};

CheckResult CheckTemplateFiles(FsFileSystem* fs, const std::string& path) {
    FsFile check;
    FileHeaderInfo checkDAT = { 0 };
    u64 bytesread;

    fs::dirList list(path);
    unsigned listcount = list.getCount();

    if (listcount == 0) {
        return CheckResult::NoTemplate;
    }
    for (unsigned i = 0; i < listcount; i++)
    {
        /*
        if (fs::pathIsFiltered(path + list.getItem(i)))
            continue;
        */

        //skip over landname.dat
        if (list.getItem(i) == "landname.dat")
            continue;

        if (list.isDir(i))
        {
            std::string tobechecked = path + list.getItem(i) + "/";
            CheckResult chkres = CheckTemplateFiles(fs, tobechecked);
            if (chkres != CheckResult::Success) {
                return chkres;
            }

        }
        else
        {
            char pathbuffer[FS_MAX_PATH];
            memset(pathbuffer, 0, FS_MAX_PATH);
            std::string tobechecked = path + list.getItem(i);
            std::snprintf(pathbuffer, FS_MAX_PATH, tobechecked.c_str());
            fsFsOpenFile(fs, pathbuffer, FsOpenMode_Read, &check);
            fsFileRead(&check, 0, &checkDAT, 0x10, FsReadOption_None, &bytesread);
            if (checkDAT.SaveRevision != REVISION_SAVE || checkDAT.Major != REVISION_MAJOR || checkDAT.Minor != REVISION_MINOR) {
                //fatalThrow(checkDAT.Major);
                return CheckResult::WrongRevision;
            }
        }
    }
    return CheckResult::Success;
}

Check Checker() {
    Check checkvar;

    u64 pid = 0, tid = 0, bid = 0;
    pmdmntGetApplicationProcessId(&pid);
    pminfoGetProgramId(&tid, pid);
    DmntCheatProcessMetadata metadata;
    dmntchtGetCheatProcessMetadata(&metadata);
    memcpy(&bid, metadata.main_nso_build_id, 0x8);

    tsl::elm::Element *warning;

    //fix endianess of hash
    bid = __builtin_bswap64(bid);

    bool isACNH = false;
    if (tid == TID) isACNH = true;

    //dream check
    u32 dreamstrval;
    u16 IsDreamingBed = 0;
    //[[[main+3DCC9E8]+10]+130]+60
    u64 mainAddr = util::FollowPointerMain(0x3DCC9E8, 0x10, 0x130, 0xFFFFFFFFFFFFFFFF) + 0x60;
    dmntchtReadCheatProcessMemory(mainAddr, &dreamstrval, sizeof(u32));
    dmntchtReadCheatProcessMemory(mainAddr + EventFlagOffset + (346 * 2), &IsDreamingBed, sizeof(u16));

    if (isACNH) {
        if (bid != BID) {

            std::snprintf(ret, 100, "BID 0x%016lX", bid);
            const char* description = ret;

            warning = new tsl::elm::CustomDrawer([description](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
                renderer->drawString("Version missmatch:", false, 110, 340, 25, renderer->a(0xFFFF));
                renderer->drawString(description, false, 55, 375, 25, renderer->a(0xFFFF));
                });

            checkvar.check_result = CheckResult::WrongBID;
            checkvar.elm = warning;
            return checkvar;
        }
#if DEBUG_FC
        else {
            if (!checkedFiles) {
                FsFileSystem fsSdmc;
                fsdevMountSdmc();
                fsOpenSdCardFileSystem(&fsSdmc);
                /*
                for (u8 i = 1; i < 8; i++) {
                    fs::addPathFilter(LUNA_TEMPLATE_DIR + "Villager" + std::to_string(i));
                }
                */
                CheckResult templatefiles = CheckTemplateFiles(&fsSdmc, LUNA_TEMPLATE_DIR);
                //fs::freePathFilters();

                //player check
                if (templatefiles == CheckResult::Success && dreamstrval != 0x0 && IsDreamingBed != 0x0) {
                    std::vector<bool> players(0x8, false);
                    static std::string playernumbers = "";
                    static char stringbuffer[0x100] = { 0 };
                    for (u8 i = 0; i < 8; i++) {
                        u64 offset = i * GSavePlayerVillagerAccountSize;
                        u128 AccountUID = 0;
                        dmntchtReadCheatProcessMemory(mainAddr + GSavePlayerVillagerAccountOffset + offset, &AccountUID, 0x10);
                        if (AccountUID != 0) {
                            if (access(std::string("/config/luna/template/Villager" + std::to_string(i)).c_str(), F_OK) == -1)
                                templatefiles = CheckResult::NotEnoughPlayers;
                            players[i] = true;
                            playernumbers.append((playernumbers.empty() ? "" : ", ") + std::to_string(i));
                        }
                    }
                    std::snprintf(stringbuffer, 0x100, std::string("Villager" + playernumbers).c_str());

                    if (templatefiles == CheckResult::NotEnoughPlayers) {
                        warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                            renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
                            renderer->drawString("template needs to include", false, 60, 340, 25, renderer->a(0xFFFF));
                            renderer->drawString(stringbuffer, false, 60, 375, 25, renderer->a(0xF06F));
                            });

                        checkvar.check_result = templatefiles;
                        checkvar.elm = warning;
                        fsFsClose(&fsSdmc);
                        fsdevUnmountDevice("sdmc");
                        return checkvar;
                    }
                }

                if (templatefiles != CheckResult::Success) {
                    if (templatefiles == CheckResult::NoTemplate) {
                        warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                            renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
                            renderer->drawString("No valid template found.", false, 70, 340, 25, renderer->a(0xFFFF));
                            });

                        checkvar.check_result = templatefiles;
                        checkvar.elm = warning;
                        fsFsClose(&fsSdmc);
                        fsdevUnmountDevice("sdmc");
                        return checkvar;
                    }
                    else if (templatefiles == CheckResult::WrongRevision) {
                        warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                            renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
                            renderer->drawString("Either wrong save revision,", false, 60, 340, 25, renderer->a(0xFFFF));
                            renderer->drawString("or template is encrypted.", false, 65, 375, 25, renderer->a(0xFFFF));
                            });

                        checkvar.check_result = templatefiles;
                        checkvar.elm = warning;
                        fsFsClose(&fsSdmc);
                        fsdevUnmountDevice("sdmc");
                        return checkvar;
                    }
                }
                else checkedFiles = true;
            }
#endif
//if debug is enabled, dont perform dream check
#if !DEBUG_UI
            //if there is a town and is in dream
            if (dreamstrval == 0x0 || IsDreamingBed == 0x0) {
                warning = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
                    renderer->drawString("\uE008", false, 180, 250, 90, renderer->a(0xFFFF));
                    renderer->drawString("you didnt pass", false, x + 95, 340, 25, renderer->a(0xFFFF));
                    renderer->drawString("the simp check.", false, x + 95, 375, 25, renderer->a(0xFFFF));
                    });
                checkvar.check_result = CheckResult::NoDream;
                checkvar.elm = warning;
                return checkvar;
            }
#endif
            checkvar.check_result = CheckResult::Success;
            return checkvar;
        }
    }
    else {
#if DEBUG_UI
        std::snprintf(ret, 100, "TID 0x%lX", tid);
        const char* description = ret;
#else
        const char* description = "Please open ACNH!";
#endif
        warning = new tsl::elm::CustomDrawer([description](tsl::gfx::Renderer* renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString("\uE150", false, 180, 250, 90, renderer->a(0xFFFF));
            renderer->drawString(description, false, 60, 340, 25, renderer->a(0xFFFF));
            });

        checkvar.check_result = CheckResult::WrongTID;
        checkvar.elm = warning;
        return checkvar;
    }
}

class OverlayTest : public tsl::Overlay {
public:
    // libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
    virtual void initServices() override {
        pminfoInitialize();
        dmntchtInitialize();
        dmntchtForceOpenCheatProcess();
        timeInitialize();
    }  // Called at the start to initialize all services necessary for this Overlay

    virtual void exitServices() override {
        pminfoExit();
        dmntchtExit();
        timeExit();
    }  // Called at the end to clean up all services previously initialized

    // Called before overlay wants to change from invisible to visible state
    virtual void onShow() override {}
    // Called before overlay wants to change from visible to invisible state
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        
        //create file struct if not found
        tsl::hlp::doWithSDCardHandle([]{
            if (access(LUNA_DIR, F_OK) == -1) {
                mkdir(LUNA_DIR, 0777);
            }
            if (access("/config/luna/dump", F_OK) == -1) {
                mkdir("/config/luna/dump", 0777);
            }
            if (access("/config/luna/template", F_OK) == -1) {
                mkdir("/config/luna/template", 0777);
                return;
            }
        });

        Check cr = Checker();
        if (cr.check_result == CheckResult::Success) return initially<SelectionGui>(this);
        else return initially<GuiError>(cr.elm);
    }
};

int main(int argc, char** argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
