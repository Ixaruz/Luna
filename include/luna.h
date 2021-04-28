#define LUNA_DIR							"/config/luna"
#define LUNA_DUMP_DIR						LUNA_DIR "/dump/"
#define LUNA_TEMPLATE_DIR					LUNA_DIR "/template/"

#define TID									0x01006F8002326000

//*1.10.0 specific stuff*//

#define BID									0x8379D9E90FF19B0F
#define MAINFILE_SIZE						8836464

#define REVISION_MAJOR						0x7D001
#define REVISION_MINOR						0x7D004
#define REVISION_SAVE						19

//**********************//

#define MAJOR_VERSION						0
#define MINOR_VERSION						4
#define REVISION_VERSION					0
#define STRINGIFY(x)						#x
#define TOSTRING(x)							STRINGIFY(x)
#define STRING_VERSION						"" TOSTRING(MAJOR_VERSION) "." TOSTRING(MINOR_VERSION) "." TOSTRING(REVISION_VERSION) ""

#define DEBUG								0
#define DEBUG_UI							0
#define DEBUG_FS							0
#define DEBUG_FC							1