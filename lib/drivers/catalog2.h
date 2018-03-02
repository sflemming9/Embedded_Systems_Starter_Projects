#pragma once

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <canrouter.h>
#include <stdbool.h>

// Operation identifiers
typedef enum {
    CAT_OP_GET_TYPEID = 0x0,
    CAT_OP_GET_LEN = 0x1,
    CAT_OP_GET_FLAGS = 0x2,
    CAT_OP_GET_NAMELEN = 0x3,
    CAT_OP_GET_NAME = 0x4,
    CAT_OP_GET_VALUE = 0x5,
    CAT_OP_GET_HASH = 0x6,
    CAT_OP_GET_ANNOUNCE = 0x7,
    CAT_OP_SET_ANNOUNCE = 0xE,
    CAT_OP_SET_VALUE = 0xF
} Cat_OpIdE;

// Type identifiers ID values
typedef enum {
    CAT_TID_UINT8 = 0x0,
    CAT_TID_INT8 = 0x1,
    CAT_TID_UINT16 = 0x2,
    CAT_TID_INT16 = 0x3,
    CAT_TID_UINT32 = 0x4,
    CAT_TID_INT32 = 0x5,
    CAT_TID_CHAR = 0x6,
    CAT_TID_FLOAT = 0x7,
    CAT_TID_DOUBLE = 0x8,
    CAT_TID_BOOL = 0x9,
    CAT_TID_ENUM = 0xA,
    CAT_TID_BITFIELD = 0xB,
    CAT_TID_INT64 = 0xC,
    CAT_TID_UINT64 = 0xD
} Cat_TypeIdE;

// Required catalog entry IDs
typedef enum {
    CAT_REQVID_VERSION = 0x0,
    CAT_REQVID_CATHASH = 0x1,
    CAT_REQVID_VIDARRAY = 0x2,
    CAT_REQVID_BOARDNAME = 0x3,
    CAT_REQVID_FWHASH = 0xD,
    CAT_REQVID_HWREV = 0xE,
    CAT_REQVID_SERIALNUM = 0xF,
    CAT_REQVID_SYSTIME = 0x10,
    CAT_REQVID_REBOOTFLAG = 0x11,
    CAT_REQVID_REBOOTSTRING = 0x12,
    CAT_REQVID_SAVEPARAMS = 0x13
} Cat_ReqVidE;

// Catalog entry flag bitfield
typedef __packed struct CatFlagsT {
    unsigned int padding:28;
    bool announced:1;
    bool transient:1;
    bool nonvolatile:1;
    bool writable:1;
} Cat_FlagsBf;

// Catalog CAN identifier bitfield
typedef __packed struct Cat_IdBfT {
    unsigned int opId:4;
    unsigned int index:8;
    unsigned int varId:8;
    unsigned int deviceId:8;
    unsigned int isCatalog:1;
    unsigned int padding:3;
} Cat_IdBf;

typedef union CatIdT {
    uint32_t asInt;
    Cat_IdBf asFields;
} Cat_Id;

// Forward declaration of structs. Only necessary in order to allow function
// pointers to include struct arguments.
typedef struct BoardInfoT BoardInfo;
typedef struct RequiredVariablesT RequiredVariables;
typedef struct CatalogEntryT CatalogEntry;
typedef struct CatalogT Catalog;

struct BoardInfoT {
   char* boardName;
   uint32_t hwRev;
   uint32_t serial;
   uint32_t deviceId;
};

struct CatalogEntryT {
    Cat_TypeIdE typeId;
    Cat_FlagsBf flags;
    uint32_t length;
    uint32_t varId;
    const char* name;
    void* varptr;
    bool (*writeCallback)(Catalog* cat, int index, void* arg);
    void* writeArg;
    bool (*readCallback)(Catalog* cat, int index, void* arg);
    void* readArg;
};

struct CatalogT {
    Can* can;
	struct BoardInfoT* boardInfo;
	struct CatalogEntryT* entries[256];
	uint8_t varIds[256];
    xQueueHandle rxQueue;
    uint32_t varCount;
    uint32_t catHash;
    uint32_t swHash;
};

// Adds a new catalog entry. Assumes length 1, transient, and sets announce
// based on the provided interval.
CatalogEntry* Cat_AddEntry(
        Catalog *cat,
        const char *name,
        void *varptr,
        Cat_TypeIdE typeId,
        uint8_t varId,
        uint32_t announceInterval);

// Initialize a new catalog
void Cat_InitCatalog(
        Catalog* cat,
        Can* can,
        BoardInfo* boardInfo,
        uint32_t fNum);

// Immediately send an entry's complete value array
void Cat_PublishValues(Catalog *cat, CatalogEntry *entry);

// Immediately send only the chosen index of an entry's value array
void Cat_PublishElement(Catalog *cat, CatalogEntry *entry, int32_t index);

/*// Print all vars in an Eric-friendly format
void Cat_PrintVars(Catalog *cat);*/
