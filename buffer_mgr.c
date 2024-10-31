#include "storage_mgr.h"
#include "buffer_mgr.h"
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
  
#define Max_Filename_Size (200)
#define Timestamp_secPosition (0)
#define Timestamp_usecPosition (1)

static int *Pin_Counter;
static bool *Dirty_Flag;
static int *Frame_Number;
static char **ThePage_Frame = NULL;
static int numWrite_IO = 0;
static int numRead_IO = 0;
static char Copy_File_Name[Max_Filename_Size];
static int Current_Frame_Position;
static long int Current_TimeStamp[2];
static long int **Frame_TimeStamp;


static int getFrameNumOfOldest(BM_BufferPool *const bm);
static void updateTimestamp(void);

RC shutdownBufferPool(BM_BufferPool *const bm)
{


    forceFlushPool(bm); /Flushing the pool/
    int i = 0;
    /* Freeing the memory*/
    while (i < bm->numPages)
    {
        free(ThePage_Frame[i]); /* Freeing the page frame */
        i++;
    }

    /* Freeing other variables */
    free(ThePage_Frame);
    free(Frame_Number);
    free(Dirty_Flag);
    free(Pin_Counter);

    int j = 0;

    /* FOR LRU STRATEGY ONLY*/
    if (bm->strategy == RS_LRU)
    {
        while (j < bm->numPages)
        {
            free(Frame_TimeStamp[j]);
            j++;
        }
        free(Frame_TimeStamp);
    }
    return RC_OK;
}

RC handleFileOperations(SM_FileHandle *fh, BM_BufferPool *const bm)
{
    return openPageFile(bm->pageFile, fh);
}
// Helper function to write the page to disk
RC writePageToFile(SM_FileHandle *fh, BM_PageHandle *const page)
{
    RC rc = ensureCapacity(page->pageNum + 1, fh);
    if (rc == RC_OK)
    {
        rc = writeBlock(page->pageNum, fh, page->data);
    }
    return rc;
}
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    SM_FileHandle fh;
    RC returnCode;

    // Open the page file
    returnCode = handleFileOperations(&fh, bm);
    if (returnCode != RC_OK)
    {
        return returnCode;
    }

    // Write the page to disk
    returnCode = writePageToFile(&fh, page);

    // Close the file and update write IO count
    closePageFile(&fh);
    numWrite_IO++;

    return returnCode;
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
    SM_FileHandle fh;
    int a, b; // Using int a = 0; int b = 0;
    int returncode_forRC_OK = RC_OK;

    // Check whether the requested page number already exists in the page frame
    for (a = 0; a < bm->numPages; a++)
    {
        b++;
        if (Frame_Number[a] == pageNum)
        {
            break;
        }
        b--;
    }
    b = 0;

    // Only active when strategy is FIFO
    if (bm->strategy == RS_FIFO)
    {
        b++;
        // Already exists in the page frame
        if (a < bm->numPages)
        {
            // Update existing page frame
            page->pageNum = pageNum;
            page->data = ThePage_Frame[a];
            Pin_Counter[a] += 1;
            Current_Frame_Position = a;
        }
        else  // Not exists in the page frame
        {
            // According to the FIFO strategy to calculate the next frame position
            Current_Frame_Position = (Current_Frame_Position + 1) % bm->numPages;

            for (a = 0; a < bm->numPages; a++)
            {
                // When the calculated frame is still in use, calculate the next frame number through FIFO strategy again
                if (Pin_Counter[Current_Frame_Position] > 0)
                {
                    Current_Frame_Position = (Current_Frame_Position + 1) % bm->numPages;
                }
                else
                {
                    // When the calculated frame is not in use anymore, check whether it is a dirty page or not
                    if (Dirty_Flag[Current_Frame_Position] == TRUE)
                    {
                        // If dirty, before overwritten, write its data to disk
                        page->pageNum = Frame_Number[Current_Frame_Position];
                        page->data = ThePage_Frame[Current_Frame_Position];
                        returncode_forRC_OK = forcePage(bm, page);

                        // Clear the dirty state when the flush operation is complete
                        if (returncode_forRC_OK == RC_OK)
                        {
                            Dirty_Flag[Current_Frame_Position] = FALSE;
                        }
                    }

                    openPageFile(bm->pageFile, &fh);
                    // Judging from the totalNumPages, the target page has been written to the page file
                    if (pageNum + 1 <= fh.totalNumPages)
                    {
                        // Read the target page from disk
                        returncode_forRC_OK = readBlock(pageNum, &fh, ThePage_Frame[Current_Frame_Position]);
                        numRead_IO += 1;
                    }
                    closePageFile(&fh);

                    // Update frame data and pageNum with the latest info after dirty data stored to disk
                    page->pageNum = pageNum;
                    page->data = ThePage_Frame[Current_Frame_Position];
                    Frame_Number[Current_Frame_Position] = pageNum;
                    Pin_Counter[Current_Frame_Position] += 1;

                    break;
                }
            }
        }
    }
    // Only active when strategy is LRU
    else if (bm->strategy == RS_LRU)
    {
        b++;
        // Already exists in the page frame
        if (a < bm->numPages)
        {
            // Update existing page frame
            page->pageNum = pageNum;
            page->data = ThePage_Frame[a];
            Pin_Counter[a] += 1;

            // Update the existing page frame's timestamp
            updateTimestamp();
            Frame_TimeStamp[a][Timestamp_secPosition] = Current_TimeStamp[Timestamp_secPosition];
            Frame_TimeStamp[a][Timestamp_usecPosition] = Current_TimeStamp[Timestamp_usecPosition];
        }
        else
        {
            // Get the pool frame number of the page which stored in buffer pool was last accessed or not accessed
            Current_Frame_Position = getFrameNumOfOldest(bm);
            // Check whether the calculated frame is a dirty page or not
            if (Dirty_Flag[Current_Frame_Position] == TRUE)
            {
                // If dirty, before overwritten, write its data to disk
                page->pageNum = Frame_Number[Current_Frame_Position];
                page->data = ThePage_Frame[Current_Frame_Position];
                returncode_forRC_OK = forcePage(bm, page);
                // Clear the dirty state when the flush operation is complete
                if (returncode_forRC_OK == RC_OK)
                {
                    Dirty_Flag[Current_Frame_Position] = FALSE;
                }
            }

            openPageFile(bm->pageFile, &fh);
            // Judging from the totalNumPages, the target page has been written to the page file
            if (pageNum + 1 <= fh.totalNumPages)
            {
                // Read the target page from disk
                returncode_forRC_OK = readBlock(pageNum, &fh, ThePage_Frame[Current_Frame_Position]);
                numRead_IO += 1;
            }
            closePageFile(&fh);
            // Update frame data and pageNum with the latest info after dirty data stored to disk
            page->pageNum = pageNum;
            page->data = ThePage_Frame[Current_Frame_Position];

            Frame_Number[Current_Frame_Position] = pageNum;
            Pin_Counter[Current_Frame_Position] += 1;
            // Update the existing page frame's timestamp
            updateTimestamp();
            Frame_TimeStamp[Current_Frame_Position][Timestamp_secPosition] = Current_TimeStamp[Timestamp_secPosition];
            Frame_TimeStamp[Current_Frame_Position][Timestamp_usecPosition] = Current_TimeStamp[Timestamp_usecPosition];
        }
    }
    return returncode_forRC_OK;
}


RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    int i = 0;
    int Counter_dirty = 0;

    while (i < bm->numPages)
    {
        Counter_dirty++;

        if (Frame_Number[i] == page->pageNum)
        {
            Counter_dirty = 0;
            Dirty_Flag[i] = TRUE;
        }
        i++;
        Counter_dirty--;
    }
    Counter_dirty = 0;
    return RC_OK;
}

// Helper function to unpin a specific page
RC unpinSpecificPage(BM_PageHandle *const page, int index)
{
    if (Frame_Number[index] == page->pageNum && Pin_Counter[index] > 0)
    {
        Pin_Counter[index] -= 1; // Decrease the pin count for the page
    }
    return RC_OK;
}
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    RC returnCode = RC_OK;

    // Loop through all pages in the buffer pool
    for (int i = 0; i < bm->numPages; i++)
    {
        // Unpin the page if it matches the page number in the frame
        returnCode = unpinSpecificPage(page, i);
        if (returnCode != RC_OK)
        {
            break;
        }
    }

    return returnCode;
}

// Helper function to check if the timestamp of the current frame is older than the oldest found so far
bool isOlderFrame(int currentFrame, int oldestFrame)
{
    if (Frame_TimeStamp[oldestFrame][Timestamp_secPosition] > Frame_TimeStamp[currentFrame][Timestamp_secPosition])
    {
        return true;
    }

    if (Frame_TimeStamp[oldestFrame][Timestamp_secPosition] == Frame_TimeStamp[currentFrame][Timestamp_secPosition] &&
        Frame_TimeStamp[oldestFrame][Timestamp_usecPosition] > Frame_TimeStamp[currentFrame][Timestamp_usecPosition])
    {
        return true;
    }

    return false;
}
// Helper function to check if the frame can be overwritten (not pinned)
bool isFrameFreeToUse(int index)
{
    return Pin_Counter[index] <= 0;
}
int getFrameNumOfOldest(BM_BufferPool *const bm)
{
    int oldestFrame = 0;

    // Traverse all frames to find the oldest (unpinned) frame
    for (int i = 0; i < bm->numPages; i++)
    {
        if (isFrameFreeToUse(i))
        {
            if (isOlderFrame(i, oldestFrame))
            {
                oldestFrame = i;

                // If timestamp is 0, no need for further traversal, this frame is unused
                if (Frame_TimeStamp[oldestFrame][Timestamp_secPosition] == 0)
                {
                    break;
                }
            }
        }
    }

    return oldestFrame;
}

RC flushDirtyPage(BM_BufferPool *const bm, BM_PageHandle *h, int index)
{
    RC rc = RC_OK;

    // Flush the frame contents to disk if the page frame is dirty
    if (Dirty_Flag[index] == TRUE)
    {
        h->pageNum = Frame_Number[index];
        h->data = ThePage_Frame[index];

        // Write the dirty frame back to disk
        rc = forcePage(bm, h);

        // If successful, clear the dirty flag
        if (rc == RC_OK)
        {
            Dirty_Flag[index] = FALSE;
        }
    }

    return rc;
}
RC forceFlushPool(BM_BufferPool *const bm)
{
    int i = 0;
    RC returnCode = RC_OK;
    BM_PageHandle *h = MAKE_PAGE_HANDLE();

    while (i < bm->numPages)
    {
        returnCode = flushDirtyPage(bm, h, i);

        if (returnCode != RC_OK)
        {
            break;
        }
        i++;
    }

    free(h);
    return returnCode;
}

int getNumWriteIO(BM_BufferPool *const bm)
{
    // int Counter_write = 0;
    return numWrite_IO;
}

PageNumber *getFrameContents(BM_BufferPool *const bm)
{
    // int Frame_Size = 0;
    return Frame_Number;
}

int getNumReadIO(BM_BufferPool *const bm)
{
    // int Counter_read = 0;
    return numRead_IO;
}

// Helper function to initialize page frames and associated variables
void initPageFrames(int numPages)
{
    ThePage_Frame = (char **)malloc(sizeof(char *) * numPages);
    Frame_Number = (int *)malloc(sizeof(int) * numPages);
    Dirty_Flag = (bool *)malloc(sizeof(bool) * numPages);
    Pin_Counter = (int *)malloc(sizeof(int) * numPages);

    // Initialize each frame's details
    for (int i = 0; i < numPages; i++)
    {
        ThePage_Frame[i] = (char *)malloc(sizeof(char) * PAGE_SIZE);
        memset(ThePage_Frame[i], '\0', PAGE_SIZE);
        Frame_Number[i] = NO_PAGE;
        Dirty_Flag[i] = FALSE;
        Pin_Counter[i] = 0;
    }
}

// Helper function to initialize LRU-specific variables
void initLRU(int numPages)
{
    Frame_TimeStamp = (long int **)malloc(sizeof(long int *) * numPages);

    for (int i = 0; i < numPages; i++)
    {
        Frame_TimeStamp[i] = (long int *)malloc(sizeof(long int) * 2);
        Frame_TimeStamp[i][Timestamp_secPosition] = 0;
        Frame_TimeStamp[i][Timestamp_usecPosition] = 0;
    }
}

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy, void *stratData)
{

    // Initialize page frames, dirty flags, and pin counters
    initPageFrames(numPages);

    // Initialize read and write counters
    numRead_IO = 0;
    numWrite_IO = 0;
    Current_Frame_Position = -1;

    // Copy file name and set buffer pool properties
    strcpy(Copy_File_Name, pageFileName);
    bm->pageFile = Copy_File_Name;
    bm->numPages = numPages;
    bm->strategy = strategy;
    bm->mgmtData = stratData;

    // Initialize LRU-specific variables if the strategy is LRU
    if (bm->strategy == RS_LRU)
    {
        initLRU(numPages);
    }

    return RC_OK;
}

int *getFixCounts(BM_BufferPool *const bm)
{
    // int Fixcount_counter = 0;
    return Pin_Counter;
}

bool *getDirtyFlags(BM_BufferPool *const bm)
{
    // int dirty_counter = 0;
    return Dirty_Flag;
}

void getCurrentTimestamp(struct timeval *tmv)
{
    gettimeofday(tmv, NULL);
}
void storeTimestamp(const struct timeval *tmv)
{
    Current_TimeStamp[Timestamp_secPosition] = tmv->tv_sec;
    Current_TimeStamp[Timestamp_usecPosition] = tmv->tv_usec;
}
void updateCounter(int *counter, int value)
{
    *counter += value;
}
void updateTimestamp(void)
{
    struct timeval tmv;
    int timestamp_counter = 0;

    getCurrentTimestamp(&tmv);
    updateCounter(&timestamp_counter, 1);

    storeTimestamp(&tmv);

    updateCounter(&timestamp_counter, -1);
}
