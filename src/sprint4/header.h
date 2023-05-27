/**
 * @file header.h
 * @author Kylian Thezenas - Valentin Racaud--Minuzzi - Leo d'Amerval
 * @brief file containing the global variable for the project FAR of the 6th semester of the engineering cycle of Polytech Montpellier
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/**
 * @brief maximum number of clients
 */
#define MAX_CLIENTS 4
/**
 * @brief maximum size of the messages exchanged
 */
#define MAX_LENGTH 100
/**
 * @brief maximum size of the pseudo
 */
#define PSEUDO_LENGTH 20
/**
 * @brief folder of the server containing its files
 */
#define PATH_SERVER_FILES "./files_Server"
/**
 * @brief folder of the clients containing their files
 */
#define PATH_CLIENT_FILES "./files_Client/"
/**
 * @brief size of the chunks of the files
 */
#define CHUNK_SIZE 512
/**
 * @brief maximum number of channels
 */
#define MAX_CHANNELS 10

/**
 * @brief structure of files
 */
typedef struct{
    char filename[MAX_LENGTH];
    int size;
    int index_sender;
} file_info;

/**
 * @brief structure of clients
 */
typedef struct{
    int dSC;
    int dSF;
    char pseudo[PSEUDO_LENGTH];
    int channel;
} clientConnecte;

/**
 * @brief structure of channels
 */
typedef struct{
    int index;
    char name[MAX_LENGTH];
} channel;