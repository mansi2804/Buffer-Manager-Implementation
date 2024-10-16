#include <unistd.h>
#include "storage_mgr.h"
#include <stdio.h>
#include "dberror.h"
#include <stdlib.h>

/**
 * @brief This function initialize the storage manager to make it ready to be used.
 */
void initStorageManager(void)
{
    FILE *file = NULL;
    printf("Setup of the storage manager has been configured in a successful way and the manager is now up and running.\n");
}


/**
 * @brief Creates a new page file with a single page initialized to zero bytes.
 * @param fileName Created file should have this name.
 * @return RC_OK if successful.
 *         RC_FILE_NOT_FOUND if creation fails.
 */
RC createPageFile(char *fileName)
{
    // Opening the file using fopen function in r+ mode.
    FILE *file= fopen(fileName, "r+");
    // If the file exists then the task is already done so return RC_OK message.
    if (file != NULL)
    {
        RC_message = "File is already present there";
        return RC_OK;
    }
    // Else we have to create the file
    else {
        printf("The file %s does not exist!\n",fileName);
        // Again opening the file in w mode.
        FILE *file = fopen(fileName,"w");
        // file pointer will be NULL if file could not be opened.
        if(file == NULL) {
            printf("The file %s could not be opened!\n",fileName);
        }
        // Memory block is formed using calloc to initilaize a buffer.
        SM_PageHandle *buffer=(SM_PageHandle *)calloc(sizeof(char),PAGE_SIZE);
        // When there is error in initializing buffer then the buffer pointer will be NULL.
        if(buffer==NULL) {
            printf("Memory allocation error!\n");
            return RC_WRITE_FAILED;
        }
        // Using fwrite we will add elements of buffer to the file.
        size_t written= fwrite(buffer,sizeof(char),PAGE_SIZE,file);
        // fwrite() function will return the total number of elements successfully written.
        // if this number is less than PAGE_SIZE then write failed.
        if(written!=PAGE_SIZE) {
            printf("Write error!\n");
            return RC_WRITE_FAILED;
        }
        // If nothing has been returned till now means we have successfully completed  the task of creating a page file.
        printf("Write operation completed\n");
        // Memory block will be cleared using fclose and free the buffer using free function.
        fclose(file);
        free(buffer);
        return RC_OK;


    }
}


/**
 * @brief Opens an existing page file and initializes the file handle.
 *
 * @param fileName This the name of the file that wil be opened.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_NOT_FOUND if file doesn't exist.
 */
RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
    FILE *file = fopen(fileName, "r+");

    if (file == NULL){
        printf("The file %s could not be opened!\n",fileName);
        return RC_FILE_NOT_FOUND;
    }
    // Initializing the fileName of fhandle
    else{
        fHandle->fileName = fileName;
    }
    // Getting the current position of the file pointer using ftell.
    long current_pos=ftell(file);
    // ftell will return -1 if the file could not be opened.
    if(current_pos==-1) {
        printf("The file %s 's current position can't be determined\n",fileName);
        return RC_FILE_NOT_FOUND;
    }
    // Storing the current position of the file pointer.
    else {
        printf("The file %s has been opened!\n",fileName);
        printf("File's current position is %ld\n",current_pos);
        fHandle->curPagePos = current_pos;
    }
    // using fseek function to reach the end of the file.
    fseek(file,0,SEEK_END);
    long end_pos=ftell(file);
    // Strong the file pointer in mgmtInfo
    fHandle->mgmtInfo = file;
    if(end_pos==-1) {
        printf("The file %s 's end position can't be determined\n",fileName);
        return RC_FILE_NOT_FOUND;
    }
    else {
        printf("The file %s has been opened!\n",fileName);
        printf("File's end position is %ld\n",end_pos);
        fHandle->totalNumPages = end_pos/PAGE_SIZE;
    }
    return RC_OK;


}


/**
 * @brief Closes an open page file and releases associated resources.
 *
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_NOT_FOUND if file handle is invalid.
 */
RC closePageFile(SM_FileHandle *fHandle)
{
    FILE *file=fHandle->mgmtInfo;
    if(file==NULL) {
        printf("The file %s could not be opened!\n",fHandle->fileName);
        return RC_FILE_NOT_FOUND;
    }
    // Closing the page using fclose function.
    int checkClose=fclose(file);
    // fclose will return 0 if the file has been closed successfully of else it's not closed.
    if (checkClose==0) {
        printf("The file %s has been closed!\n",fHandle->fileName);
        return RC_OK;

    }
    else {
        printf("The file %s could not be closed!\n",fHandle->fileName);
        return RC_FILE_NOT_FOUND;
    }

}


/**
 * @brief This function will delete the page file form the system.
 *
 * @param fileName This the name of the file that wil be deleted.
 * @return RC_OK if successful.
 *         RC_FILE_NOT_FOUND if file doesn't exist.
 */
RC destroyPageFile(char *fileName)
{

    // Page file is being deleted using remove function.
    int removeCheck=remove(fileName);
    // If the file is deleted then remove function will return 0.
    if(removeCheck==0) {
        printf("The file %s has been removed!\n",fileName);
        return RC_OK;
    }
    else {
        printf("The file %s could not be removed!\n",fileName);
        return RC_FILE_NOT_FOUND;
    }
}


/**
 * @brief This function will read the file form the system and will store it in the memory.
 *
 * @param pageNum Exact page no that will be read form the file.
 * @param memPage It is the pointer to the memory buffer that will be used to store the block data.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_READ_NON_EXISTING_PAGE if page doesn't exist.
 */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE *file = fopen(fHandle->fileName, "r+");
    if(fseek(file, pageNum*PAGE_SIZE, SEEK_SET) == 0) {
        printf("The file %s has been seeked!\n",fHandle->fileName);
        // we are using fread function to read the page
        int read_char=fread(memPage, sizeof(char), PAGE_SIZE/sizeof(char), file);
        // freed will return no of bytes it read so it should match will the PAGE_SIZE as we are reading the whole page.
        if(read_char==PAGE_SIZE/sizeof(char)) {
            printf("The file %s has been read!\n",fHandle->fileName);
            fHandle->curPagePos = pageNum;
            fclose(file);
            return RC_OK;
        }
        else {
            printf("The file %s could not be read!\n",fHandle->fileName);
            return RC_READ_NON_EXISTING_PAGE;
        }
    }

    else {
        printf("The file %s could not be read!\n",fHandle->fileName);
        return RC_READ_NON_EXISTING_PAGE;

    }
}


/**
 * @brief Retrieves the current page position in the file.
 *
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return The current page position in the file.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 */
int getBlockPos(SM_FileHandle *fHandle)
{
    if (fHandle == NULL ) {
        printf("File can't be initialized because file handle is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // Storing the current page position to return it whenever needed.
    return fHandle->curPagePos;
}


/**
 * @brief First block of the file will be read into memory.
 *
 * @param memPage It is the pointer to the memory buffer that will be used to store the first block's data.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_READ_NON_EXISTING_PAGE if file is empty.
 */
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // Calling the readBlock function with parameter of page no set to 0 to read the first block.
    return readBlock(0, fHandle, memPage);
}


/**
 * @brief Reads the previous block relative to the current page position.
 *
 * @param memPage It is the pointer to the memory buffer that will be used to store the previous block's data.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_READ_NON_EXISTING_PAGE if no previous page.
 */
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // Calling the readBlock function with parameter of page no set to the previous page of current page.
    return readBlock(fHandle->curPagePos-1, fHandle, memPage);
}


/**
 * @brief Reads the current block in the file into memory.
 *
 * @param memPage It is the pointer to the memory buffer that will be used to store the current block's data.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_READ_NON_EXISTING_PAGE if position is invalid.
 */
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // Calling the readBlock function with parameter of page no set to current page position.
    return readBlock(fHandle->curPagePos, fHandle, memPage);
}


/**
 * @brief Reads the next block relative to the current page position.
 *
 * @param memPage It is the pointer to the memory buffer that will be used to store the next block's data.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_READ_NON_EXISTING_PAGE if no next page.
 */
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // Calling the readBlock function with parameter of page no set to next page of the file.
    return readBlock(fHandle->curPagePos+1, fHandle, memPage);
}


/**
 * @brief Reads the last block of the file into memory.
 *
 * @param memPage It is the pointer to the memory buffer that will be used to store the last block's data.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_READ_NON_EXISTING_PAGE if file is empty.
 */
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // Calling the readBlock function with parameter of page no set to last block.
    return readBlock(fHandle->totalNumPages-1, fHandle, memPage);
}


/**
 * @brief Writes a block to a specific page number in the file.
 *
 * @param pageNum The exact page number where the block is going to be written.
 * @param memPage It is the pointer to the memory buffer on which data is to be written.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_WRITE_FAILED if write operation fails.
 */
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    FILE *file = (FILE*) fHandle->mgmtInfo;
    if (file==NULL) {
        printf("The file %s could not be opened!\n",fHandle->fileName);
        return RC_FILE_NOT_FOUND;

    }
    // pageNum should be greater than or equal to zero and total pages in fhandle should be greater the pageNum
    if( pageNum>=0 && pageNum<fHandle->totalNumPages) {
        if(fseek(file, pageNum*PAGE_SIZE, SEEK_SET)==0) {
            printf("The file %s has been seeked!\n",fHandle->fileName);
            fseek(file, pageNum * PAGE_SIZE, SEEK_SET);
            size_t writtenBytes = fwrite(memPage, 1, PAGE_SIZE, file);
            if (writtenBytes != PAGE_SIZE) {
                perror("Error writing page to file");
                return RC_WRITE_FAILED;
            }
            fHandle->curPagePos = pageNum;
            // going to the end of the page file and storing the total no of pages.
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            if (fileSize == -1) {
                perror("Error determining file size");
                return RC_FILE_NOT_FOUND;
            }

            fHandle->totalNumPages = fileSize / PAGE_SIZE;

        }
        else {
            printf("The file %s could not be seeked!\n",fHandle->fileName);
            return RC_WRITE_FAILED;
        }

    }
    else {
        printf("The file %s could not be opened!\n",fHandle->fileName);
        return RC_WRITE_FAILED;
    }
    return RC_OK;
}


/**
 * @brief Writes the current block in the file from memory.
 *
 * @param memPage It is the pointer to the memory buffer that will contain the data to be written.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_WRITE_FAILED if write operation fails.
 */

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
    if (fHandle == NULL || memPage == NULL) {
        printf("File can't be initialized because file handle or memory page is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    int nowBlock = getBlockPos(fHandle);
    // FILE *file =(FILE*) fHandle->mgmtInfo;
    if(nowBlock==-1) {
        printf("The file %s could not be opened!\n",fHandle->fileName);
        return RC_FILE_HANDLE_NOT_INIT;
    }
    // nowBlock is written in the fHandle page file.
    if(writeBlock(nowBlock, fHandle, memPage)==RC_OK) {
        printf("The file %s could be written!\n",fHandle->fileName);
        return RC_OK;
    }
    else {
        printf("The file %s could not be written!\n",fHandle->fileName);
        return RC_WRITE_FAILED;

    }
}


/**
 * @brief Empty block will be appended in the end of the file system and the size will increase.
 *
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if successful.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_WRITE_FAILED if append operation fails.
 */

RC appendEmptyBlock(SM_FileHandle *fHandle)
{
    if (fHandle == NULL) {
        printf("File can't be initialized because file handle is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE *file =(FILE*) fHandle->mgmtInfo;
    if(fseek(file, 0, SEEK_END)==0) {
        printf("The file %s could be seeked!\n",fHandle->fileName);
        SM_PageHandle newPage=(SM_PageHandle)calloc(1,PAGE_SIZE);
        if(newPage==NULL) {
            printf("The file %s 's memory allocation failed.\n",fHandle->fileName);
            return RC_WRITE_FAILED;
        }
        else {
            printf("The file %s could be opened!\n",fHandle->fileName);
            if(fwrite(newPage, sizeof(char), PAGE_SIZE/sizeof(char), file)==PAGE_SIZE/sizeof(char)) {
                printf("The file %s could be written!\n",fHandle->fileName);
                fHandle->totalNumPages += 1;
                free(newPage);
                return RC_OK;

            }
            else {
                printf("The file %s could not be written!\n",fHandle->fileName);
                return RC_WRITE_FAILED;
            }
        }
    }
    else {
        printf("The file %s could not be seeked!\n",fHandle->fileName);
        return RC_WRITE_FAILED;
    }
}


/**
 * @brief this function will chack weather the file a the required no of pages and will append blocks if needed.
 * @param numberOfPages The no pages that the file must be having.
 * @param fHandle It is the pointer of the file on which the operation will be performed.
 * @return RC_OK if the file has required no of pages.
 *         RC_FILE_HANDLE_NOT_INIT if the file has not be initialized.
 *         RC_WRITE_FAILED if the append operation failed.
 */

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
    if (fHandle == NULL ) {
        printf("File can't be initialized because file handle is null.\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fHandle->fileName == NULL) {
        printf("The file name is incorrect or is not initialized in file handle\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if(numberOfPages<0) {
        printf("The page range is out of bound");
        return RC_WRITE_FAILED;
    }
    int pages=fHandle->totalNumPages;
    // fHandle should have the specified no of pages or else add pages till the specified number is not reached.
    for(int i=0;i<numberOfPages-pages;i++) {
        appendEmptyBlock(fHandle);

    }
    return RC_OK;
}