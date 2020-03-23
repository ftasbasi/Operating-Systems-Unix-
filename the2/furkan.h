#include <semaphore.h>
#include <stdlib.h> 
#include <unistd.h> 

typedef struct Minerr {
    unsigned int ID;
    OreType oreType;
    unsigned int capacity;
    unsigned int current_count;
    unsigned int Im;//interval
    unsigned int Rm; //limit of production
} typeMiner;
typedef struct Transporterr {
    unsigned int ID;
    OreType *carry;
    unsigned int It;//interval
    
} typeTransporter;
typedef struct Smelterr {
    unsigned int ID;
    OreType oreType;
    unsigned int loading_capacity;
    unsigned int waiting_ore_count;
    unsigned int total_produce;
    unsigned int Is;//interval
    
} typeSmelter;

typedef struct Foundryy {
    unsigned int ID;
    unsigned int loading_capacity;
    unsigned int waiting_iron;
    unsigned int waiting_coal;
    unsigned int total_produce;
    unsigned int Finterval;//interval
    
} typeFoundry;
OreType setOre(int ore) {
    switch (ore) {
        case 0:
            return IRON;
        case 1:
            return COPPER;
        case 2:
            return COAL;
    }
}
void *threadFOUNDRYY(void *arg){

    return NULL;
}