#define LUNA_DIR							"/config/luna"
#define LUNA_DUMP_DIR						LUNA_DIR "/dump/"
#define LUNA_TEMPLATE_DIR					LUNA_DIR "/template/"

#define TID									0x01006F8002326000

//*1.11.1 specific stuff*//

#define BID									0x1CD4B05D52E20EAA
#define BID_LAST							0x2768322A8F2F45F1
#define MAINFILE_SIZE						8836464

#define REVISION_MAJOR						0x7E001
#define REVISION_MINOR						0x7E001
#define REVISION_SAVE						21
#define REVISION_SAVE_LAST					20

//**********************//

#define MAJOR_VERSION						0
#define MINOR_VERSION						5
#define REVISION_VERSION					3
#define STRINGIFY(x)						#x
#define TOSTRING(x)							STRINGIFY(x)
#define STRING_VERSION						"" TOSTRING(MAJOR_VERSION) "." TOSTRING(MINOR_VERSION) "." TOSTRING(REVISION_VERSION) ""

#define DEBUG								1
#define DEBUG_UI							0
#define DEBUG_FS							0
#define DEBUG_FC							1