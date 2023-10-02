#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "finders.c"
#include "generator.c"
#include "biome_tree.c"
#include "layers.c"
#include "noise.c"

typedef struct 
{
    Pos first, second;
} DoublePos;

typedef struct 
{
    DoublePos regionCoords;
    int dimension;
    Pos candidates[10];
    Pos positions[10];
    int candidatesCount;
    int positionsCount;
} StructData;

DoublePos origCoords = {{-96, -96}, {96, 96}};
int STRUCTS[] = {Bastion, Fortress, End_City};
const int MC = MC_1_16_1;
const int UPPER_BITS_TO_CHECK = 1;

int structureChecker(int lower48, int STRUCTS[], int structureIndex, int MC, DoublePos origCoords, StructData data[], int result, Pos* bastionCoordinates, Pos* fortressCoordinates, Pos* endCityCoordinates);

int structureChecker(int lower48, int STRUCTS[], int structureIndex, int MC, DoublePos origCoords, StructData data[], int result, Pos* bastionCoordinates, Pos* fortressCoordinates, Pos* endCityCoordinates) 
{
    Pos p;
    int i = structureIndex; // Use the correct data element corresponding to the structure being checked
    data[i].candidatesCount = 0;
    int currentStructure = STRUCTS[structureIndex];

    StructureConfig currentStructureConfig;
        if (!getStructureConfig(STRUCTS[i], MC, &currentStructureConfig)) 
        {
            printf("ERROR: Structure #%d in the STRUCTS array cannot exist in the specified version.\n", i);
            exit(1);
        }

        switch (currentStructureConfig.regionSize) 
        {
            case 32:
                data[i].regionCoords.first.x = origCoords.first.x >> 9;
                data[i].regionCoords.first.z = origCoords.first.z >> 9;
                data[i].regionCoords.second.x = origCoords.second.x >> 9;
                data[i].regionCoords.second.z = origCoords.second.z >> 9;
                break;
            case 1:
                data[i].regionCoords.first.x = origCoords.first.x >> 4;
                data[i].regionCoords.first.z = origCoords.first.z >> 4;
                data[i].regionCoords.second.x = origCoords.second.x >> 4;
                data[i].regionCoords.second.z = origCoords.second.z >> 4;
                break;
            default:
                data[i].regionCoords.first.x = (origCoords.first.x / (currentStructureConfig.regionSize << 4)) - (origCoords.first.x < 0);
                data[i].regionCoords.first.z = (origCoords.first.z / (currentStructureConfig.regionSize << 4)) - (origCoords.first.z < 0);
                data[i].regionCoords.second.x = (origCoords.second.x / (currentStructureConfig.regionSize << 4)) - (origCoords.second.x < 0);
                data[i].regionCoords.second.z = (origCoords.second.z / (currentStructureConfig.regionSize << 4)) - (origCoords.second.z < 0);
                break;
        }

        data[i].dimension = currentStructureConfig.properties & STRUCT_NETHER ? DIM_NETHER :
                            currentStructureConfig.properties & STRUCT_END    ? DIM_END :
                                                                                DIM_OVERWORLD;

    for (int regX = data[i].regionCoords.first.x; regX <= data[i].regionCoords.second.x; ++regX) 
    {
        for (int regZ = data[i].regionCoords.first.z; regZ <= data[i].regionCoords.second.z; ++regZ) 
        {
            if (!getStructurePos(currentStructure, MC, lower48, regX, regZ, &p)) continue;

            if ((regX == data[i].regionCoords.first.x && p.x < origCoords.first.x) ||
                (regX == data[i].regionCoords.second.x && p.x > origCoords.second.x) ||
                (regZ == data[i].regionCoords.first.z && p.z < origCoords.first.z) ||
                (regZ == data[i].regionCoords.second.z && p.z > origCoords.second.z)) continue;

            data[i].candidates[data[i].candidatesCount] = p;
            data[i].candidatesCount++;
        }
    }

    if (!data[i].candidatesCount) 
    {
        result = 1;
    } 
    else if (structureIndex == 0) 
    {
        result = 2;
        for (int j = 0; j < data[i].candidatesCount; ++j) 
        {
            bastionCoordinates[j].x = data[i].candidates[j].x;
            bastionCoordinates[j].z = data[i].candidates[j].z;
        }
    } 
    else if (structureIndex == 1) 
    {
        result = 2;
        for (int j = 0; j < data[i].candidatesCount; ++j) 
        {
            fortressCoordinates[j].x = data[i].candidates[j].x;
            fortressCoordinates[j].z = data[i].candidates[j].z;
        }
    }
    else if (structureIndex == 2)
    {
        result = 2;
        for (int j = 0; j < data[i].candidatesCount; ++j) 
        {
            endCityCoordinates[j].x = data[i].candidates[j].x;
            endCityCoordinates[j].z = data[i].candidates[j].z;
        }
    }
    return result;
}

Pos mainIslandGateway(uint64_t lower48)
{
	uint64_t rng = 0;               // some initial value idk
	setSeed(&rng, lower48);            // new Random(seed);
	int rngResult = nextInt(&rng, 20); // nextInt(20);
	double angle = 2.0 * (-1 * PI + 0.15707963267948966 * (rngResult));
	double gatewayX = (int)(96.0 * cos(angle));
    double gatewayZ = (int)(96.0 * sin(angle));
	Pos result;
	result.x = gatewayX;
	result.z = gatewayZ;

    return result;
}

Pos linkedGateway(uint64_t lower48)
{
	uint64_t rng = 0;               // Initializtion
	setSeed(&rng, lower48);            // new Random(seed);
	int rngResult = nextInt(&rng, 20); // nextInt(20);
	double angle = 2.0 * (-1 * PI + 0.15707963267948966 * (rngResult));
	double gatewayX = (1024.0 * cos(angle));
    double gatewayZ = (1024.0 * sin(angle));
    (int) gatewayX >> 4;
    (int) gatewayZ >> 4;

    int noiseResultSum = 0;

    EndNoise en;
    setEndSeed(&en, MC_1_16_1, lower48);

    SurfaceNoise sn;
    initSurfaceNoise(&sn, DIM_END, lower48);

    int emptyChunk = 0;

    for (int n = 0; n < 16; n++) //Checking towards from the main end island to see if there are blocks (in case the original vector plopped me in the middle of a huge island)
    {
        int blockCheckResult = 0;

        for (int xIterator = 0; xIterator < 16; xIterator++)
        {
            for (int zIterator = 0; zIterator < 16; zIterator++)
            {
                blockCheckResult = getSurfaceHeightEnd(MC_1_16_1, lower48, gatewayX + xIterator, gatewayZ + zIterator);
                if (blockCheckResult > 0)
                {
                    goto chunkHasBlocks;
                }
            }
        }
        
        chunkHasBlocks:

        if (blockCheckResult > 0) //Move forward a chunk
        {
            gatewayX = (double) gatewayX - (16.0 * cos(angle)); 
            gatewayZ = (double) gatewayZ - (16.0 * sin(angle));
        }
        else
        {
            emptyChunk = 1;
            (int) gatewayX >> 4;
            (int) gatewayZ >> 4;
            break; //Empty chunk found
        }   
    }

    if (emptyChunk != 1)
    {
        for (int n = 0; n < 16; n++) //Checking away from the main end island to see if there are blocks (in case the original vector plopped me in the middle of the void)
        {
            int blockCheckResult = 0;

            for (int xIterator = 0; xIterator < 16; xIterator++)
            {
                for (int zIterator = 0; zIterator < 16; zIterator++)
                {
                    blockCheckResult = getSurfaceHeightEnd(MC_1_16_1, lower48, gatewayX + xIterator, gatewayZ + zIterator);
                    if (blockCheckResult > 0)
                    {
                        goto chunkHasBlocks2;
                    }
                }
            }
            
            chunkHasBlocks2:

            if (blockCheckResult > 0) //Move forward a chunk
            {
                gatewayX = (double) gatewayX + (16.0 * cos(angle)); 
                gatewayZ = (double) gatewayZ + (16.0 * sin(angle));
            }
            else
            {
                (int) gatewayX >> 4;
                (int) gatewayZ >> 4;
                break; //Empty chunk found
            }   
        }
    }

	Pos result;
	result.x = gatewayX;
	result.z = gatewayZ;
    return result;
}

Pos goodMainIslandGateway[12] = 
{
	{ 96,  0}, {  0, 96},
	{-96,  0}, {  0,-96}
};

int findMainIslandGateway(Pos innerGateway, Pos goodMainIslandGateway[]);


int findMainIslandGateway(Pos innerGateway, Pos goodMainIslandGateway[]) 
{
    for (int i = 0; i < 11; i++) 
	{
        if (goodMainIslandGateway[i].x == innerGateway.x && goodMainIslandGateway[i].z == innerGateway.z)
		{
            return 1;  // Match found, return a non-zero value
        }
    }
    return 0;  // No match found, return 0
}


int main(int argc, char **argv) 
{
    FILE *fp;
    char *inputPath = "seedRange.txt";

    for (int i = 1; i < argc; i++) 
    {
        if (strcmp(argv[i], "--input") == 0) 
        {
            inputPath = argv[i + 1];
        }
    }

    if (inputPath == NULL) 
    {
        printf("Error: --input argument is required.\n");
        return 1;
    }

    fp = fopen(inputPath, "r");
    if (fp == NULL) 
    {
        printf("Error: Unable to open input file.\n");
        return 1;
    }

    uint64_t startingStructureSeed, endingStructureSeed;
    if (fscanf(fp, "%" SCNu64 "%" SCNu64, &startingStructureSeed, &endingStructureSeed) != 2) 
    {
        printf("Error: Unable to read seeds from file.\n");
        fclose(fp);
        return 1;
    }
    fclose(fp);
    
    const int numberOfStructs = sizeof(STRUCTS) / sizeof(*STRUCTS);
    StructData data[numberOfStructs];

    int i = 0;
    data[i].candidatesCount = 0;
    data[i].positionsCount = 0;
    
    Pos bastionCoordinates[4];
    Pos fortressCoordinates[4];
    Pos endCityCoordinates[9];

    Generator g;
    setupGenerator(&g, MC, 0);

    int biome = 0;
    int result = 0;
    uint64_t seed;

    for (uint64_t lower48 = startingStructureSeed; lower48 < endingStructureSeed; ++lower48) 
    {
        Pos innerGateway = mainIslandGateway(lower48);
        int innerGatewayResult;
        innerGatewayResult = findMainIslandGateway(innerGateway, goodMainIslandGateway);
        if (innerGatewayResult == 1)
        {
        }
        else
        {
            goto nextStructureSeed;
        }

        applySeed(&g, DIM_END, lower48);
        SurfaceNoise sn;
        initSurfaceNoise(&sn, 1, lower48);
        Pos outerGateway;
        outerGateway = linkedGateway(lower48);
        origCoords = (DoublePos) {{-96 + outerGateway.x, -96 + outerGateway.z}, {96 + outerGateway.x, 96 + outerGateway.z}};                        
        
        for (uint64_t upper16 = 0; upper16 < UPPER_BITS_TO_CHECK; ++upper16) 
        {
                seed = lower48 | (upper16 << 48);
        }
        printf("Seed: %" PRId64 "", seed);
        printf("    Outer Gateway: %d %d\n", outerGateway.x, outerGateway.z);

        nextStructureSeed:

        for (int i = 0; i < numberOfStructs; ++i) 
        {
            data[i].candidatesCount = 0;
            data[i].positionsCount = 0;
        }
        continue;
    }
    fclose(fp);
    printf("Done\n");
    return 0;
}
