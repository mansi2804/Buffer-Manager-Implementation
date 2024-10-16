# 📚 **Advanced Database Organization**
## 🎯 **Assignment No. 2**

  
 
## 🏆 **Group No. 24**

### **Group Members:**
- **Mansi Patil**
  *Student ID:* A20549858
- **Priyansh Salian**
  *Student ID:* A20585026
- **Soham Joshi**
  *Student ID:* A20586602


---

### 🛠️ **Group Members' Involvement**

- **Mansi Patil**
      - `RC shutdownBufferPool`
      - 'handleFileOperations'
      - 'unpinSpecificPage'
      - `RC unpinPage`
      - `RC forceFlushPool`
      - `getNumWriteIO`
      - `getCurrentTimestamp`

    

- **Priyansh Salian**
      - 'writePageToFile'
      - `RC forcePage`
      - `RC pinPage`
      - `getFrameContents`
      - `getFixCounts`
      - `RC flushDirtyPage`
      - `StoreTimestamp`
     
     

- **Soham Joshi**
      - `RC initBufferPool`
      - `RC markDirty`      
      - `getDirtyFlags`
      - `getNumReadIO`
      - 'initPageFrames'
      - `initLRU`
      - `updateTimestamp`

---

### 📊 **Contribution Overview**

Each group member contributed equally to the assignment, with active participation and shared responsibilities throughout the project. 🎉

---

### 📂 **Files Included**

The folder includes the following files:
1. buffer_mgr_stat.c
2. buffer_mgr_stat.h
3. buffer_mgr.c
4. buffer_mgr.h
5. dberror.c
6. dberror.h
7. storage_mgr.c
8. storage_mgr.h
9. dt.h
10. test_assign2_1.c
11. test_assign2_2.c
12. test_helper.h
13. Makefile
14. README.md

---

### 📋 **Overview of the Buffer Manager Implementation**

The Buffer Manager is a module that facilitates reading and writing blocks from a disk file and managing their access in a buffer pool. It also recommends using suitable page replacement strategies to write modified (dirty) data back to the disk when the buffer pool is full, and a new page needs to be loaded.

---

### 🚀 **How to Run the Code**

1.Open the terminal and clone the repository from BitBucket to your desired location.
2.Navigate to the folder where the repository was cloned, and you will find a folder named "assign2_buffer_manager."
3.Inside this folder, use the "make" command by typing "make test1" and "make test2" to build the necessary files.
4.Next, execute the test cases by running the commands "make run_test1" and "make run_test2."
5.To clean up the executable files and reset the project for a fresh run, use the "make clean" command.

---

### 🧩 Function Explanations

Below are the explanations of the functions used in our code:

shutdownBufferPool(): 
The shutdownBufferPool function flushes the buffer pool to ensure all dirty pages are written to disk, then frees the memory allocated for the page frames, tracking variables (frame number, dirty flags, pin counters), and any additional resources used by the LRU strategy. Finally, it returns a success code (RC_OK).

handleFileOperations():
The handleFileOperations function opens the page file associated with the buffer pool by calling openPageFile. 

writePageToFile(): 
The writePageToFile function ensures there is enough capacity in the file for the specified page, and if successful, writes the page's data to the disk using writeBlock.

forcePage():
The forcePage function opens the page file using handleFileOperations, writes the specified page to disk via writePageToFile, and then closes the file. Afterward, it increments the write I/O count (numWrite_IO) and returns the appropriate status code.

pinPage():
The pinPage function loads a page into the buffer pool using FIFO or LRU strategies. It first checks if the requested page is already in the buffer. If it exists, the function updates the page's data, increases the pin count, and in the case of LRU, updates its timestamp. If the page isn't in the buffer, it finds an available frame using FIFO or LRU. If the frame contains a dirty page, it writes the page to disk before loading the new one. The function then reads the page from disk, updates the frame with the new page's data, and adjusts the pin count and frame details.

markDirty():
The markDirty function marks a page in the buffer pool as dirty, indicating it has been modified. It iterates through the buffer pool's page frames to find the one matching the specified page number. Once the matching frame is found, it sets the corresponding Dirty_Flag to TRUE. The Counter_dirty variable is incremented and decremented during the process but does not play a critical role in the logic. Finally, the function returns RC_OK to indicate success.

unpinSpecificPage():
The unpinSpecificPage function decreases the pin count of a specific page in the buffer pool. It checks if the page number at the given index matches the specified page and whether the pin count is greater than zero. If both conditions are met, the pin count is reduced by 1. The function then returns RC_OK to indicate success.

unpinPage():
The unpinPage function goes through each page in the buffer pool and calls unpinSpecificPage to decrease the pin count if the page is found and currently pinned. The isOlderFrame function checks if one frame's timestamp is older than another. The isFrameFreeToUse function sees if a frame is free by making sure its pin count is zero or less. Then, getFrameNumOfOldest looks through all the frames to find the oldest unpinned one by comparing timestamps, and it stops early if it finds a completely unused frame. This helps decide which page to replace when needed.

flushDirtyPage():
The flushDirtyPage function is designed to write back any changes made to a page in the buffer pool to disk if that page is marked as "dirty." The function first checks if the page at the specified index is dirty by looking at the Dirty_Flag array. If the page is dirty, it updates the BM_PageHandle structure h with the page number and data from the corresponding frame.Next, it calls the forcePage function to write the dirty page's contents back to disk. If the write operation is successful (i.e., rc remains RC_OK), the function then clears the dirty flag for that page by setting Dirty_Flag[index] to FALSE, indicating that the page is now clean. Finally, the function returns the result code, which reflects whether the operation was successful or not.

forceFlushPool(): 
The forceFlushPool function iterates through all the pages in the buffer pool. For each page, it calls flushDirtyPage, which checks if the page is dirty and writes it to disk if necessary. If any operation fails (returns a non-zero code), the loop breaks, and the function returns the result code. It also allocates a BM_PageHandle structure to temporarily hold page data and frees this memory at the end.

getNumWriteIO(): 
The getNumWriteIO function simply returns the total number of write I/O operations that have occurred, which is tracked by the global variable numWrite_IO.

getFrameContents():
The getFrameContents function returns the current contents of the page frames by providing access to the Frame_Number array, which holds the page numbers currently in the buffer pool.

getNumReadIO():
The getNumReadIO Similar to getNumWriteIO, this function returns the total number of read I/O operations tracked by numRead_IO.

initPageFrames():
The initPageFrames helper function initializes the various data structures needed for managing the pages in the buffer pool. It allocates memory for the arrays that hold page data (ThePage_Frame), frame numbers (Frame_Number), dirty flags (Dirty_Flag), and pin counters (Pin_Counter). Each page frame is initialized to zero, and the dirty flags are set to FALSE, indicating no pages are dirty initially.

initLRU():
The initLRU function initializes the data structures needed for implementing the Least Recently Used (LRU) page replacement strategy. It allocates a two-dimensional array to store timestamps for each page frame, which are initialized to zero. These timestamps help track when each page was last accessed, allowing the LRU algorithm to identify which page to evict when a new page is needed.

initBufferPool():
The initBufferPool function initializes the buffer pool, which is a cache of pages to improve database performance. It takes parameters for the buffer pool structure (bm), the name of the page file, the number of pages, the replacement strategy (e.g., LRU), and additional strategy-specific data. The function begins by initializing page frames, dirty flags, and pin counters through the initPageFrames function. It also initializes read and write counters to zero and sets the current frame position to -1. The page file name and buffer pool properties, such as the number of pages and the replacement strategy, are set. If the chosen replacement strategy is LRU (Least Recently Used), it calls initLRU to set up LRU-specific variables. Finally, it returns a status code indicating success (RC_OK).

getFixCounts():
The getFixCounts function returns the current pin counts for the pages in the buffer pool. It provides access to the Pin_Counter array, which tracks how many references (or "fixes") each page has.

getDirtyFlags():
Similar to getFixCounts, the getDirtyFlags function returns the dirty flags for the pages in the buffer pool. It provides access to the Dirty_Flag array, which indicates whether each page has been modified since it was last written to disk.

getCurrentTimestamp():
 The getCurrentTimestamp function retrieves the current time and stores it in a struct timeval variable (tmv). It uses the gettimeofday function, which populates the tmv structure with the current time in seconds and microseconds.

storeTimestamp(): 
The storeTimestamp function stores the current timestamp in a global variable (Current_TimeStamp). It takes a pointer to a struct timeval variable (tmv) and saves the seconds and microseconds values from tmv into the respective positions of Current_TimeStamp.

updateCounter():
The updateCounter utility function updates a given counter by a specified value. It takes a pointer to an integer (counter) and the value to add (value), updating the counter's value accordingly.

updateTimestamp(): 
The updateTimestamp function updates the current timestamp by first retrieving the current time using getCurrentTimestamp. It then increments a timestamp_counter, stores the retrieved timestamp, and finally decrements the timestamp_counter. The timestamp_counter appears to serve as a way to track how many times the timestamp has been updated, though it is not used beyond this function.



### 🙏 Gratitude

Thank you for getting involved in this project! Your time is truly appreciated. 🚀 🚀

---
