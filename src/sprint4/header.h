#define MAX_CLIENTS 4
#define MAX_LENGTH 100
#define PSEUDO_LENGTH 20
#define PATH_SERVER_FILES "./files_Server"
#define PATH_CLIENT_FILES "./files_Client/"
#define CHUNK_SIZE 512

// structure of files
typedef struct{
    char filename[MAX_LENGTH];
    int size;
    int index_sender;
} file_info;

// structure of users
typedef struct{
    int dSC;
    int dSF;
    char pseudo[PSEUDO_LENGTH];
    int channel;
} clientConnecte;