#include <catalog2.h>
#include <bsp.h>
#include <csp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// Private constants
static const uint32_t kCAT_MAX_STRLEN = 2048;
static const uint32_t kCAT_VERSION = 1;

// Forward function declarations
static uint32_t Cat_GetStrChunkedLen(const char* src);
static uint32_t Cat_GetEntryHash(CatalogEntry* entry, bool reset);

// Lookup table of type sizes to simplify handling
uint8_t typeSizes[16] = {
    sizeof(uint8_t), // 0: uint8
    sizeof(int8_t), // 1: int8
    sizeof(uint16_t), // 2: uint16
    sizeof(int16_t), // 3: int16
    sizeof(uint32_t), // 4: uint32
    sizeof(int32_t), // 5: int32
    sizeof(char) * 8, // 6: string
    sizeof(float), // 7: float
    sizeof(double), // 8: double
    sizeof(bool), // 9: bool
    sizeof(uint32_t), // 10: enumeration (uint32)
    sizeof(uint32_t), // 11: bitfield (uint32)
    sizeof(int64_t), // 12: int64
    sizeof(uint64_t), // 13: uint64
    0, // 14: unused
    0 // 15: unused
};

// Encapsulate needed material for operation handlers
typedef struct HandlerDataT {
    Catalog* cat;
    CatalogEntry* target;
    Cat_Id rxId;
    CanRxMsg* rxMsg;
    CanTxMsg* txMsg;
} HandlerData;

// Operation handlers
// Returns true if the packet assembled into data->txMsg is OK to send
// Returns false if no transmission should take place

// Handle type ID get requests
static bool Cat_GetTypeIdHandler(HandlerData* d){
    d->txMsg->DLC = sizeof(uint32_t);
    memcpy(d->txMsg->Data, &(d->target->typeId), sizeof(uint32_t));
    return true;
}

// Handle length get requests
static bool Cat_GetLengthHandler(HandlerData* d){
    d->txMsg->DLC = sizeof(uint32_t);
    uint32_t length = d->target->length;
    memcpy(d->txMsg->Data, &length, sizeof(length));
    return true;
}

// Handle flags get requests
static bool Cat_GetFlagsHandler(HandlerData* d){
    d->txMsg->DLC = sizeof(d->target->flags);
    memcpy(d->txMsg->Data, &(d->target->flags), d->txMsg->DLC);
    return true;
}

// Handle name length get requests
static bool Cat_GetNameLengthHandler(HandlerData* d){
    uint32_t nameLength = strnlen(d->target->name, kCAT_MAX_STRLEN - 1) + 1;
    d->txMsg->DLC = sizeof(nameLength);
    memcpy(d->txMsg->Data, &nameLength, sizeof(nameLength));
    return true;
}

// Handle name get requests
static bool Cat_GetNameHandler(HandlerData* d){
    uint32_t index = d->rxId.asFields.index;
    uint32_t numNameChunks = Cat_GetStrChunkedLen(d->target->name);
    uint32_t nameLength = strnlen(d->target->name, kCAT_MAX_STRLEN - 1) + 1;
    // Bounds check the requested array location; bail on failure
    if(index < numNameChunks){
        char* src = &(((char*)d->target->name)[index * 8]);
        d->txMsg->DLC = MIN(8, nameLength - (index * 8));
        memset(d->txMsg->Data, 0, 8);
        strncpy((char*)(d->txMsg->Data), src, 8);
        return true;
    } else {
        return false;
    }
}

// Handle value get requests
static bool Cat_GetValueHandler(HandlerData* d){
    Cat_PublishElement(d->cat, d->target, d->rxId.asFields.index);
    return false;
}

// Handle hash get requests
static bool Cat_GetHashHandler(HandlerData* d){
    d->txMsg->DLC = sizeof(uint32_t);
    uint32_t hashValue = Cat_GetEntryHash(d->target, true);
    memcpy(d->txMsg->Data, &hashValue, sizeof(hashValue));
    return true;
}

// TODO: Implement these two functions once we actually have an announcer
static bool Cat_GetAnnounceHandler(HandlerData* d){
    return true;
}
static bool Cat_SetAnnounceHandler(HandlerData* d){
    return true;
}

// Handle value set requests
static bool Cat_SetValueHandler(HandlerData* d){
    if(!(d->target->flags.writable))
        return false;
    uint32_t elemSize = typeSizes[d->target->typeId];
    uint32_t index = d->rxId.asFields.index;
    uint32_t offset = elemSize * index;
    d->txMsg->DLC = elemSize;
    if(d->rxId.asFields.index < d->target->length)
        memcpy(&(((char*)d->target->varptr)[offset]), d->txMsg->Data, elemSize);
    if(d->target->writeCallback != NULL)
        (d->target->writeCallback)(d->cat, index, d->target->writeArg);
    return false;
}

// Lookup table of operation handler function pointers
bool (*Cat_OpHandler[16])(HandlerData* d) = {
    Cat_GetTypeIdHandler,
    Cat_GetLengthHandler,
    Cat_GetFlagsHandler,
    Cat_GetNameLengthHandler,
    Cat_GetNameHandler,
    Cat_GetValueHandler,
    Cat_GetHashHandler,
    Cat_GetAnnounceHandler,
    0,
    0,
    0,
    0,
    0,
    0,
    Cat_SetAnnounceHandler,
    Cat_SetValueHandler
};

// Callback to update the system time prior to its transmission
static bool Cat_UpdateSystimeEntry(Catalog* cat, int index, void* target){
    CatalogEntry* entry = (CatalogEntry*)target;
    uint64_t sysTimeUs = CSP_TotalClockCycles() * 1000000 / SystemCoreClock;
    memcpy(entry->varptr, &sysTimeUs, sizeof(uint64_t));
    return true;
}

// Reboot the CPU.
// TODO: Support writing something to SRAM to indicate cause and desired outcome
static bool Cat_Reboot(Catalog* cat, int index, void* target){
    CSP_Reboot();
    return false;
}

// Compute the size of a string as chunked
static uint32_t Cat_GetStrChunkedLen(const char* src){
    uint32_t len = strnlen(src, kCAT_MAX_STRLEN - 1) + 1;
    // Round up to the nearest chunk
    uint32_t bytesToChunk = 8 - (len % 8);
    len += (bytesToChunk < 8) ? bytesToChunk : 0;
    return len / 8;
}

// Compute a CRC32 for a catalog entry
// CRC32 engine computes a running CRC32 (Ethernet standard polynomial).
// You use it by appending chunks of words. The CRC32 result register is always
// complete and correct and may be used at any time. To start a new CRC32,
// you must reset the CRC32 data register.
static uint32_t Cat_GetEntryHash(CatalogEntry* entry, bool reset){
    if (reset)
        CRC_ResetDR();
    // First four integers are typeId, flags, length, and varId.
    // Those, plus the name (below) are relevant for hashing an entry
    uint32_t calc = CRC_CalcBlockCRC((uint32_t *)entry, 4);

    int name_space = strnlen(entry->name, kCAT_MAX_STRLEN - 1) + 1;
    int len = ((name_space + (((name_space % 4) != 0) ? 4 : 0)) / 4);
    calc = CRC_CalcBlockCRC((uint32_t *)entry->name, len);
    return calc;
}

// Compute a CRC32 for an entire catalog
uint32_t Cat_GetCatHash(Catalog* cat){
	taskENTER_CRITICAL();
    CRC_ResetDR();

    uint32_t crc = 0;
    for(int a = 0; a < cat->varCount; a++)
        crc = Cat_GetEntryHash(cat->entries[a], false);

	taskEXIT_CRITICAL();
    return crc;
}

// Compute the CRC32 of everything in flash
// TODO: We shouldn't hash parameter storage space, once it's implemented
uint32_t Cat_GetSwHash(Catalog *catalog){
    taskENTER_CRITICAL();
    uint32_t crc = 0;
    CRC_ResetDR();
    crc = CRC_CalcBlockCRC((void*)CSP_GetFlashStartAddr(), CSP_GetFlashSize());
    taskEXIT_CRITICAL();
    return crc;
}

// Save permanent entries to flash
// @TODO(Rachel, w/Sasha): Actually do this
void Cat_WriteToFlash(Catalog* catalog, void* addr);

// Restore permanent entries from flash
// @TODO(Rachel, w/Sasha): Actually do this
void Cat_ReadFromFlash(Catalog* catalog, void* addr);

// Wrapper for Cat_PublishElement that sends the entirety of an entry
void Cat_PublishValues(Catalog *cat, CatalogEntry* entry){
    for(int a = 0; a < entry->length; a++){
        Cat_PublishElement(cat, entry, a);
    }
}

// Transmits a packet with the value specified by the entry and the index,
// and puts it into the outgoing message buffer.
// Visible externally so that clients can explicitly broadcast after important
// updates.  Requires the client to have a pointer to the relevant entry.
void Cat_PublishElement(Catalog *cat, CatalogEntry *entry, int32_t index){
    Cat_Id id;
    CanTxMsg txMsg;
    bool shouldTx = true;
    uint32_t len = typeSizes[entry->typeId];

    // Bail if we're out-of-bounds
    if(!(index < entry->length))
        return;

    // Call the read callback
    if(entry->readCallback != NULL)
        shouldTx = (entry->readCallback)(cat, index, entry->readArg);

    // Bail out if we've decided not to transmit
    if(!shouldTx)
        return;

    // Prepare the outbound packet ID
    id.asFields.padding = 0;
    id.asFields.isCatalog = 1;
    id.asFields.deviceId = cat->boardInfo->deviceId;
    id.asFields.varId = entry->varId;
    id.asFields.index = index;
    id.asFields.opId = CAT_OP_GET_VALUE;

    // Populate non-ID fields
    txMsg.RTR = CAN_RTR_Data;
    txMsg.IDE = CAN_Id_Extended;
    txMsg.StdId = 0;
    txMsg.ExtId = id.asInt;
    txMsg.DLC = len;

    // Create target pointers for data pushing
    char* dest = (char*)(&txMsg.Data);
    char* src = &(((char*)entry->varptr)[index * len]);
    // Zero out our target, making sure any garbage we send is known to be 0
    memset(dest, 0, 8);
    // Copy the data into the target location
    if(entry->typeId == CAT_TID_CHAR){
        txMsg.DLC = MIN(strnlen(src, kCAT_MAX_STRLEN - 1) + 1, 8);
        strncpy(dest, src, 8);
    }else{
        memcpy(dest, src, len);
    }
    // Ship the data offboard
    CAN_Enqueue(cat->can, &txMsg, true);
}

// FreeRTOS task that watches the incoming packet queue and calls operation
// handlers out of an array of function pointers.
void Cat_RequestHandlerTask(void* pvParameters){
    Catalog *cat = (Catalog *)pvParameters;
    CanRxMsg rxMsg;
	CanTxMsg txMsg;
    txMsg.RTR = CAN_RTR_DATA;
    txMsg.IDE = CAN_ID_EXT;

    // Turn on CAN interrupts
    CAN_ConfigRxInterrupt(cat->can);
    CAN_ConfigTxInterrupt(cat->can);

	HandlerData d = {
		.cat = cat,
		.rxMsg = &rxMsg,
		.txMsg = &txMsg
	};

    while(1){
        if(xQueueReceive(cat->rxQueue, &rxMsg, portMAX_DELAY) == pdPASS){
            d.rxId.asInt = txMsg.ExtId = rxMsg.ExtId;
            d.target = cat->entries[d.rxId.asFields.varId];
            // Bail out if this is not a real entry
            if(d.target == NULL)
                continue;
            // Clear the memory that we're about to dump data in to
            memset(txMsg.Data, 0, 8);
            // Only run our operation handler if we think it exists
            if(Cat_OpHandler[d.rxId.asFields.opId] != 0){
                bool shouldTx = (*(Cat_OpHandler[d.rxId.asFields.opId]))(&d);
                if(shouldTx)
                    CAN_Enqueue(cat->can, d.txMsg, true);
            }
        }
    }
}

// Adds a new catalog entry. Assumes length 1, transient, and sets announce
// based on the provided interval.
CatalogEntry* Cat_AddEntry(
        Catalog *cat,
        const char *name,
        void *varptr,
        Cat_TypeIdE typeId,
        uint8_t varId,
        uint32_t announceInterval){
    // Allocate some space for the new entry, fail gracefully.
    CatalogEntry *entry = malloc(sizeof(CatalogEntry));
    if ((entry == NULL) || (cat->varCount >= 256))
		return NULL;

    // Create the new entry
	entry->typeId = typeId;
    entry->varId = varId;
    entry->varptr = varptr;
    entry->name = name;
    if(typeId == CAT_TID_CHAR){
        //entry->length = Cat_GetStrChunkedLen(varptr);
        entry->length = strnlen(varptr, kCAT_MAX_STRLEN-1) + 1;
    } else{
        entry->length = 1;
    }
    cat->entries[varId] = entry;
    cat->varIds[cat->varCount++] = varId;
    cat->entries[CAT_REQVID_VIDARRAY]->length = cat->varCount;

    // Set announce flags based on announce interval
    Cat_FlagsBf catFlags = {
        .announced = false,
        .transient = true,
        .nonvolatile = false,
        .writable = false
    };
    if(announceInterval > 0)
        catFlags.announced = true;
    entry->flags = catFlags;

    // Update the length of the varId array entry
    cat->entries[CAT_REQVID_VIDARRAY]->length = cat->varCount;

    // We've added a variable, so we need to re-hash the catalog
    cat->catHash = Cat_GetCatHash(cat);

    // Return a pointer to the object we just created. This is useful in case
    // the user wants to announce it.
    return entry;
}

// Populates entries required by the catalog specification
void Cat_AddRequiredEntries(Catalog* cat){
    CatalogEntry* entry;

    entry = Cat_AddEntry(cat, "CATHASH", (void*)&(cat->catHash), CAT_TID_UINT32, CAT_REQVID_CATHASH, 0);

    entry = Cat_AddEntry(cat, "VERSION", (void*)&kCAT_VERSION, CAT_TID_UINT32, CAT_REQVID_VERSION, 0);
    entry->flags.transient = false;

    entry = Cat_AddEntry(cat, "VARIDS", (void*)cat->varIds, CAT_TID_UINT8, CAT_REQVID_VIDARRAY, 0);

    entry = Cat_AddEntry(cat, "BOARDNAME", (void*)(cat->boardInfo->boardName), CAT_TID_CHAR, CAT_REQVID_BOARDNAME, 0);
    entry->flags.transient = false;

    entry = Cat_AddEntry(cat, "FWHASH", (void*)&(cat->swHash), CAT_TID_UINT32, CAT_REQVID_FWHASH, 0);
    cat->swHash = Cat_GetSwHash(cat);
    entry->flags.transient = false;

    entry = Cat_AddEntry(cat, "HWREV", (void*)&(cat->boardInfo->hwRev), CAT_TID_UINT32, CAT_REQVID_HWREV, 0);
    entry->flags.transient = false;

    entry = Cat_AddEntry(cat, "SERIAL", (void*)&(cat->boardInfo->serial), CAT_TID_UINT32, CAT_REQVID_SERIALNUM, 0);
    entry->flags.transient = false;

    entry = Cat_AddEntry(cat, "SYSTIME", malloc(sizeof(uint64_t)), CAT_TID_UINT64, CAT_REQVID_SYSTIME, 1000);
    entry->readCallback = &Cat_UpdateSystimeEntry;
    entry->readArg = (void*)entry;

    entry = Cat_AddEntry(cat, "REBOOTFLAG", malloc(sizeof(bool)), CAT_TID_BOOL, CAT_REQVID_REBOOTFLAG, 0);
    entry->flags.writable = true;
    entry->writeCallback = &Cat_Reboot;
/*
    // TODO: Add callback to save parameters
    entry = Cat_AddEntry(cat, "SAVEPARAMS", (void*)&(cat->reqVars.requestSaveParams), CAT_TID_BOOL, CAT_REQVID_SAVEPARAMS, 0);
    entry->flags.writable = true;
    */
}

// Initialize a new catalog
// Warning: we expect boardInfo to be declared globally in the BSP, so it
// should last forever.
void Cat_InitCatalog(
        Catalog* cat,
        Can* can,
        BoardInfo* boardInfo,
        uint32_t fNum){

    // Initialize the CRC32 peripheral
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

    // Initialize simple state variables
    cat->can = can;
    cat->rxQueue = xQueueCreate(16, sizeof(CanRxMsg));
    cat->boardInfo = boardInfo;
    cat->varCount = 0;

    // Clear the entry arrays
    memset(cat->entries, 0, sizeof(CatalogEntry*) * 256);
    memset(cat->varIds, 0, sizeof(uint8_t) * 256);

    // Populate the required catalog entries
    Cat_AddRequiredEntries(cat);

    // Subscribe to the packets relevant to our device
    uint32_t fMask = 0x0FF00000;
    uint32_t fId = boardInfo->deviceId << 20;
    CAN_InitHwF(can, fMask, fId, true, fNum);
    CAN_Subscribe(can, fMask, fId, true, cat->rxQueue);

    // Create the packet reception task
    xTaskCreate(Cat_RequestHandlerTask, (const signed char *)("CatRx"), 256,
            cat, tskIDLE_PRIORITY, NULL);
}

/*// Print all vars in an Eric-friendly format
void Cat_PrintVars(Catalog *cat){
    for(uint8_t a = 0; a < cat->varCount; a++){
        CatalogEntry* entry = cat->entries[cat->varIds[a]];
        printf("%s,0x%X,0x%X,type0x%X\r\n", entry->name, cat->boardInfo->deviceId, entry->varId, entry->typeId);
    }
}*/