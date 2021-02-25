#define LUNA_DIR							"/config/luna"
#define LUNA_DUMP_DIR						LUNA_DIR "/dump/"
#define LUNA_TEMPLATE_DIR					LUNA_DIR "/template/"

#define TID									0x01006F8002326000

//*1.8.0 specific stuff*//

#define BID									0x0E36C71C08334076
#define MAINFILE_SIZE						8690736 //same as 1.7.0

#define REVISION_MAJOR						0x78001
#define REVISION_MINOR						0x78001
#define REVISION_SAVE						17

//**********************//

#define MAJOR_VERSION						0
#define MINOR_VERSION						2
#define REVISION_VERSION					8
#define STRINGIFY(x)						#x
#define TOSTRING(x)							STRINGIFY(x)
#define STRING_VERSION						"" TOSTRING(MAJOR_VERSION) "." TOSTRING(MINOR_VERSION) "." TOSTRING(REVISION_VERSION) ""

#define DEBUG								0
#define DEBUG_UI							0
#define DEBUG_FS							0
#define DEBUG_FC							1
