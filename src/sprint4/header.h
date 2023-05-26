/**
 * @file header.h
 * @author Kylian Thezenas - Valentin Raccaud--Minuzzi -Leo d'Amerval
 * @brief file containing the global variable for the project FAR of the 6th semester of the engineering cycle of Polytech Montpellier
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/**
 * @brief global variable
 * 
 */
#define MAX_CLIENTS 4
#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20
#define PATH_SERVER_FILES "./files_Server"
#define PATH_CLIENT_FILES "./files_Client/"
#define CHUNK_SIZE 512

/**
 * @brief structure of files*
 * 
 * 
 */
typedef struct{
    char filename[MAX_LENGTH];
    int size;
    int index_sender;
} file_info;

/**
 * @brief structure of client
 * 
 */
typedef struct{
    int dSC;
    int dSF;
    char pseudo[PSEUDO_LENGTH];
    int channel;
} clientConnecte;