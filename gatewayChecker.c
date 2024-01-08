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

typedef struct {
    double x;
    double z;
} Vec;

typedef struct {
    bool island1;
    int x1;
    int y1;
    int z1;
    bool island2;
    int x2;
    int y2;
    int z2;
} smallEndIsland;

DoublePos origCoords = {{-96, -96}, {96, 96}};
int structs[] = {Bastion, Fortress, End_City};
const int MC = MC_1_16_1;
const int UPPER_BITS_TO_CHECK = 1;
const double gatewayAngles[20] = {0.00000, 0.30851, 0.62880, 0.94200, 1.26229, 1.58121, 1.88924, 2.20803, 2.51892, 2.83623, 3.14159, 3.45680, 3.77267, 4.08131, 4.39717, 4.71238, 5.01774, 5.33505, 5.64594, 5.96473}; // In radians

int structureChecker(int lower48, int structs[], int structureIndex, int MC, DoublePos origCoords, StructData data[], int result, Pos* bastionCoordinates, Pos* fortressCoordinates, Pos* endCityCoordinates);

int structureChecker(int lower48, int structs[], int structureIndex, int MC, DoublePos origCoords, StructData data[], int result, Pos* bastionCoordinates, Pos* fortressCoordinates, Pos* endCityCoordinates) 
{
    Pos p;
    int i = structureIndex; // Use the correct data element corresponding to the structure being checked
    data[i].candidatesCount = 0;
    int currentStructure = structs[structureIndex];

    StructureConfig currentStructureConfig;
        if (!getStructureConfig(structs[i], MC, &currentStructureConfig)) 
        {
            printf("ERROR: Structure #%d in the structs array cannot exist in the specified version.\n", i);
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

int findMainIslandGatewayID(uint64_t lower48)
{
    uint64_t rng = 0;               // some initial value idk
	setSeed(&rng, lower48);            // new Random(seed);
	int rngResult = nextInt(&rng, 20); // nextInt(20);
    return rngResult;
}

double findMainIslandGatewayAngle(uint64_t lower48)
{
    double gatewayAngle = gatewayAngles[findMainIslandGatewayID(lower48)];
    return gatewayAngle;
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

    return result; // Delete this later and replace with rngResult/findMainIslandGatewayID way of searching for specific gateways
}

smallEndIsland isSmallEndIsland(uint64_t lower48)
{
    smallEndIsland smallEndIslandInfo; //This data structure holds 8 things - a true/false for if both islands generated, and their respective xyz coords
    uint64_t rng = 0;               
	setSeed(&rng, lower48);            
	int rngResult = nextInt(&rng, lower48); //Checking for 1/14 chance to generate 
    if (rngResult > 0) //No small island in that chunk
    {
        smallEndIslandInfo.island1 = false;
    }
    else //At least 1 small island in that chunk
    {
        smallEndIslandInfo.x1 = (&rng, 16);
        smallEndIslandInfo.y1 = (&rng, 16) + 55;
        smallEndIslandInfo.z1 = (&rng, 16);
        
        rngResult = nextInt(&rng, lower48);

        if (rngResult > 0) //No second island
        {
            smallEndIslandInfo.island2 = false;
        }
        else //2nd small island generated
        {
            smallEndIslandInfo.x2 = (&rng, 16);
            smallEndIslandInfo.y2 = (&rng, 16) + 55;
            smallEndIslandInfo.z2 = (&rng, 16);
        }
    }
    return smallEndIslandInfo;
}

Pos linkedGateway(uint64_t lower48)
{
	double gatewayAngle = findMainIslandGatewayAngle(lower48);

    Vec normalizedVector;
	normalizedVector.x = cos(gatewayAngle);
    normalizedVector.z = sin(gatewayAngle); 
   
    Vec gatewayVector; // Double is %lf
    gatewayVector.x = normalizedVector.x;
    gatewayVector.z = normalizedVector.z;
    gatewayVector.x *= 1024;
    gatewayVector.z *= 1024;

    EndNoise en;
    setEndSeed(&en, MC_1_16_1, lower48);

    SurfaceNoise sn;
    initSurfaceNoise(&sn, DIM_END, lower48);

    int blockCheckResult = 0;
    bool chunkHasBlocks = false;
    bool foundChunk = false;

    bool island1 = isSmallEndIsland(lower48).island1;
    int xIsland1 = isSmallEndIsland(lower48).x1;
    int yIsland1 = isSmallEndIsland(lower48).y1;
    int zIsland1 = isSmallEndIsland(lower48).z1;
    bool island2 = isSmallEndIsland(lower48).island2;
    int xIsland2 = isSmallEndIsland(lower48).x2;
    int yIsland2 = isSmallEndIsland(lower48).y2;
    int zIsland2 = isSmallEndIsland(lower48).z2;

    for (int n = 0; n < 16; n++) //Checking towards the main end island to see if there are blocks (in case the original vector plopped me in the middle of a huge island)
    {
        if (island1)
        {
            foundChunk = true;
            break;
        }

        for (int xIterator = 0; xIterator < 16; xIterator++)
        {
            for (int zIterator = 0; zIterator < 16; zIterator++)
            {
                blockCheckResult = getSurfaceHeightEnd(MC_1_16_1, lower48, gatewayVector.x + xIterator, gatewayVector.z + zIterator);
                if (blockCheckResult > 0)
                {
                    chunkHasBlocks = true;
                    break;
                }
                else
                {
                    chunkHasBlocks = false;
                }
            }
            if (chunkHasBlocks)
            {
                break;
            }
        }

        if (chunkHasBlocks) //Move toward main island a chunk
        {
            gatewayVector.x -= (16.0 * cos(gatewayAngle)); 
            gatewayVector.z -= (16.0 * sin(gatewayAngle));
        }
        else
        {
            gatewayVector.x += (16.0 * cos(gatewayAngle)); //Going back to the last chunk with blocks in it
            gatewayVector.z += (16.0 * sin(gatewayAngle));
            foundChunk = true;
            break; //Chunk found
        }
    }

    if (!foundChunk)
    {
        gatewayVector.x = normalizedVector.x * 1024;
        gatewayVector.z = normalizedVector.z * 1024;        

        for (int n = 0; n < 16; n++) //Checking away from the main end island to see if there are blocks (in case the original vector plopped me in the middle of the void)
        {
            if (island1)
            {
                foundChunk = true;
                break;
            }

            int blockCheckResult = 0;

            for (int xIterator = 0; xIterator < 16; xIterator++)
            {
                for (int zIterator = 0; zIterator < 16; zIterator++)
                {
                    blockCheckResult = getSurfaceHeightEnd(MC_1_16_1, lower48, gatewayVector.x + xIterator, gatewayVector.z + zIterator);
                    if (blockCheckResult > 0)
                    {
                        chunkHasBlocks = true;
                        break;
                    }
                    else
                    {
                        chunkHasBlocks = false;
                    }
                }
                if (chunkHasBlocks)
                {
                    break;
                }
            }

            if (chunkHasBlocks) //Move away from main island a chunk
            {
                gatewayVector.x += (16.0 * cos(gatewayAngle)); 
                gatewayVector.z += (16.0 * sin(gatewayAngle));
            }
            else
            {
                gatewayVector.x -= (16.0 * cos(gatewayAngle)); //Going back to the last chunk with blocks in it
                gatewayVector.z -= (16.0 * sin(gatewayAngle));
                foundChunk = true;
                break; //Chunk found
            }   
        }
    }

    printf("Gateway: %lf, %lf\n", gatewayVector.x, gatewayVector.z);

    int gatewayX = round(gatewayVector.x);
    int gatewayZ = round(gatewayVector.z);
    gatewayX >>= 4;
    gatewayZ >>= 4;
    gatewayX <<= 4;
    gatewayZ <<= 4;
    gatewayX += 15;
    gatewayZ += 15;

    int surfaceHeight = 0;
    int highestSurfaceHeight = 0;
    int highestSurfaceHeightX = 0;
    int highestSurfaceHeightZ = 0;
    int noiseColumn = 0;

    for (int xIterator = -16; xIterator < 17; xIterator++)
    {
        for (int zIterator = -16; zIterator < 17; zIterator++)
        {
            surfaceHeight = getSurfaceHeightEnd(MC, lower48, gatewayX + xIterator, gatewayZ + zIterator);
            noiseColumn = getEndHeightNoise(&en, xIterator, zIterator, 0);
            
            if (surfaceHeight > highestSurfaceHeight) 
            {
                highestSurfaceHeight = surfaceHeight; // Update highestSurfaceHeight if a higher value is found
                highestSurfaceHeightX = xIterator;
                highestSurfaceHeightZ = zIterator;
                
            }
        }
    }

    gatewayX += highestSurfaceHeightX;
    gatewayZ += highestSurfaceHeightZ;

	Pos result;
	result.x = gatewayX;
	result.z = gatewayZ;

    printf("Gateway: %d, %d\n", gatewayX, gatewayZ);

   return result;
}

Pos goodMainIslandGateway[12] = //need to update
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





// Only need the functions from this program, endCity5 has the rest of the program with better syntax!






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
    
    const int numberOfStructs = sizeof(structs) / sizeof(*structs);
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
        /*Pos innerGateway = mainIslandGateway(lower48);
        int innerGatewayResult;
        innerGatewayResult = findMainIslandGateway(innerGateway, goodMainIslandGateway);
        if (innerGatewayResult == 1)
        {
        }
        else
        {
            goto nextStructureSeed;
        }*/

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

        //printf("Outer Gateway: %d %d\n", outerGateway.x, outerGateway.z);
        printf("Seed: %" PRId64 "\n", seed);

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
