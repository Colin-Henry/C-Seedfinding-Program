#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
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
    data[structureIndex].candidatesCount = 0;
    int currentStructure = STRUCTS[structureIndex];

    StructureConfig currentStructureConfig;
        if (!getStructureConfig(STRUCTS[structureIndex], MC, &currentStructureConfig)) 
        {
            printf("ERROR: Structure #%d in the STRUCTS array cannot exist in the specified version.\n", structureIndex);
            exit(1);
        }

        switch (currentStructureConfig.regionSize) 
        {
            case 32:
                data[structureIndex].regionCoords.first.x = origCoords.first.x >> 9;
                data[structureIndex].regionCoords.first.z = origCoords.first.z >> 9;
                data[structureIndex].regionCoords.second.x = origCoords.second.x >> 9;
                data[structureIndex].regionCoords.second.z = origCoords.second.z >> 9;
                break;
            case 1:
                data[structureIndex].regionCoords.first.x = origCoords.first.x >> 4;
                data[structureIndex].regionCoords.first.z = origCoords.first.z >> 4;
                data[structureIndex].regionCoords.second.x = origCoords.second.x >> 4;
                data[structureIndex].regionCoords.second.z = origCoords.second.z >> 4;
                break;
            default:
                data[structureIndex].regionCoords.first.x = (origCoords.first.x / (currentStructureConfig.regionSize << 4)) - (origCoords.first.x < 0);
                data[structureIndex].regionCoords.first.z = (origCoords.first.z / (currentStructureConfig.regionSize << 4)) - (origCoords.first.z < 0);
                data[structureIndex].regionCoords.second.x = (origCoords.second.x / (currentStructureConfig.regionSize << 4)) - (origCoords.second.x < 0);
                data[structureIndex].regionCoords.second.z = (origCoords.second.z / (currentStructureConfig.regionSize << 4)) - (origCoords.second.z < 0);
                break;
        }

        data[structureIndex].dimension = currentStructureConfig.properties & STRUCT_NETHER ? DIM_NETHER :
                                         currentStructureConfig.properties & STRUCT_END    ? DIM_END :
                                                                                             DIM_OVERWORLD;

    for (int regX = data[structureIndex].regionCoords.first.x; regX <= data[structureIndex].regionCoords.second.x; ++regX) 
    {
        for (int regZ = data[structureIndex].regionCoords.first.z; regZ <= data[structureIndex].regionCoords.second.z; ++regZ) 
        {
            if (!getStructurePos(currentStructure, MC, lower48, regX, regZ, &p)) continue;

            if ((regX == data[structureIndex].regionCoords.first.x && p.x < origCoords.first.x) ||
                (regX == data[structureIndex].regionCoords.second.x && p.x > origCoords.second.x) ||
                (regZ == data[structureIndex].regionCoords.first.z && p.z < origCoords.first.z) ||
                (regZ == data[structureIndex].regionCoords.second.z && p.z > origCoords.second.z)) continue;

            data[structureIndex].candidates[data[structureIndex].candidatesCount] = p;
            data[structureIndex].candidatesCount++;
        }
    }

    if (!data[structureIndex].candidatesCount) 
    {
        result = 1;
    } 
    else if (structureIndex == 0) 
    {
        result = 2;
        for (int candidateNumber = 0; candidateNumber < data[structureIndex].candidatesCount; ++candidateNumber) 
        {
            bastionCoordinates[candidateNumber].x = data[structureIndex].candidates[candidateNumber].x;
            bastionCoordinates[candidateNumber].z = data[structureIndex].candidates[candidateNumber].z;
        }
    } 
    else if (structureIndex == 1) 
    {
        result = 2;
        for (int candidateNumber = 0; candidateNumber < data[structureIndex].candidatesCount; ++candidateNumber) 
        {
            fortressCoordinates[candidateNumber].x = data[structureIndex].candidates[candidateNumber].x;
            fortressCoordinates[candidateNumber].z = data[structureIndex].candidates[candidateNumber].z;
        }
    }
    else if (structureIndex == 2)
    {
        result = 2;
        for (int candidateNumber = 0; candidateNumber < data[structureIndex].candidatesCount; ++candidateNumber) 
        {
            endCityCoordinates[candidateNumber].x = data[structureIndex].candidates[candidateNumber].x;
            endCityCoordinates[candidateNumber].z = data[structureIndex].candidates[candidateNumber].z;
        }
    }
    return result;
}

Pos mainIslandGateway(uint64_t lower48)
{
	uint64_t rng = 0;
	setSeed(&rng, lower48);
	int rngResult = nextInt(&rng, 20);
	double angle = 2.0 * (-1 * PI + 0.15707963267948966 * (rngResult));
	int gateway_x = (int)(96.0 * cos(angle));
    int gateway_z = (int)(96.0 * sin(angle));
	Pos result;
	result.x = gateway_x;
	result.z = gateway_z;

    return result;
}

Pos linkedGateway(uint64_t lower48)
{
	uint64_t rng = 0;
	setSeed(&rng, lower48);
	int rngResult = nextInt(&rng, 20);
	double angle = 2.0 * (-1 * PI + 0.15707963267948966 * (rngResult));
	int gateway_x = (int)(1024.0 * cos(angle));
    int gateway_z = (int)(1024.0 * sin(angle));
	Pos result;
	result.x = gateway_x;
	result.z = gateway_z;

    return result;
}

Pos goodMainIslandGatewayPositions[12] = 
{
	{ 96,  0}, { 91, 29}, { 29, 91}, {  0, 96},
	{-29, 91}, {-91, 29}, {-96,  0}, {-91,-29},
	{-29,-91}, {  0,-96}, { 29,-91}, { 91,-29},
};

int findMainIslandGateway(Pos innerGateway, Pos goodMainIslandGatewayPositions[]);

int findMainIslandGateway(Pos innerGateway, Pos goodMainIslandGatewayPositions[]) 
{
    for (int gatewayCount = 0; gatewayCount < 11; gatewayCount++) 
	{
        if (goodMainIslandGatewayPositions[gatewayCount].x == innerGateway.x && goodMainIslandGatewayPositions[gatewayCount].z == innerGateway.z)
		{
            return 1;
        }
    }
    return 0;
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
    
    const int numberOfStructures = sizeof(STRUCTS) / sizeof(*STRUCTS);
    StructData data[numberOfStructures];
    
    Pos bastionCoordinates[4];
    Pos fortressCoordinates[4];
    Pos endCityCoordinates[9];
    
    Generator g;
    setupGenerator(&g, MC, 0);

    int biome;
    int result;
    uint64_t seed;

    for (uint64_t lower48 = startingStructureSeed; lower48 < endingStructureSeed; ++lower48) 
    {
        DoublePos bastionBoundingBox = {{-96, -96}, {96, 96}};

        result = structureChecker(lower48, STRUCTS, 0, MC, bastionBoundingBox, data, result, bastionCoordinates, fortressCoordinates, endCityCoordinates);
        if (result == 1) continue;

        for (int bastionInstance = 0; bastionInstance < data[0].candidatesCount; ++bastionInstance) 
        {
            DoublePos fortressBoundingBox = {{-96 + bastionCoordinates[bastionInstance].x, -96 + bastionCoordinates[bastionInstance].z}, {96 + bastionCoordinates[bastionInstance].x, 96 + bastionCoordinates[bastionInstance].z}};
            result = structureChecker(lower48, STRUCTS, 1, MC, fortressBoundingBox, data, result, bastionCoordinates, fortressCoordinates, endCityCoordinates);
            if (result == 1) continue;

            int allChecksFailed = 1;

            Pos innerGateway = mainIslandGateway(lower48);
            int innerGatewayResult;
            innerGatewayResult = findMainIslandGateway(innerGateway, goodMainIslandGatewayPositions);
            if (innerGatewayResult != 1) continue;
            
            Pos outerGateway;
            outerGateway = linkedGateway(lower48);
            origCoords = (DoublePos) {{-96 + outerGateway.x, -96 + outerGateway.z}, {96 + outerGateway.x, 96 + outerGateway.z}};

            result = structureChecker(lower48, STRUCTS, 2, MC, origCoords, data, result, bastionCoordinates, fortressCoordinates, endCityCoordinates);
            if (result == 1) continue;

            allChecksFailed = 1;
            
            for (uint64_t upper16 = 0; upper16 < UPPER_BITS_TO_CHECK; ++upper16) 
            {
                seed = lower48 | (upper16 << 48);
                for (int structureCounter = 0; structureCounter < numberOfStructures; ++structureCounter) 
                {
                    data[structureCounter].positionsCount = 0;
                    if (g.seed != seed || g.dim != data[structureCounter].dimension) applySeed(&g, data[structureCounter].dimension, seed);
                    for (int candidatesCounter = 0; candidatesCounter < data[structureCounter].candidatesCount; ++candidatesCounter) 
                    {
                        if (!isViableStructurePos(STRUCTS[structureCounter], &g, data[structureCounter].candidates[candidatesCounter].x, data[structureCounter].candidates[candidatesCounter].z, 0) ||
                            !isViableStructureTerrain(STRUCTS[structureCounter], &g, data[structureCounter].candidates[candidatesCounter].x, data[structureCounter].candidates[candidatesCounter].z))
                            continue;
                        data[structureCounter].positions[data[structureCounter].positionsCount] = data[structureCounter].candidates[candidatesCounter];
                        data[structureCounter].positionsCount++;
                    }

                    if (data[structureCounter].positionsCount > 0) 
                    {
                        allChecksFailed = 0;
                        break;
                    }
                }

                if (!allChecksFailed) break;   
            }

            if (allChecksFailed) continue;

            biome = getBiomeAt(&g, 1, bastionCoordinates[bastionInstance].x, 64, bastionCoordinates[bastionInstance].z);
            if (biome == basalt_deltas) continue;

            biome = getBiomeAt(&g, 1, fortressCoordinates[bastionInstance].x, 64, fortressCoordinates[bastionInstance].z); 
            if (biome != soul_sand_valley) continue;

            printf("%" PRId64 "\n", seed);
            //printf("%d %d\n", outerGateway.x, outerGateway.z);
            continue;
        }

        for (int structureCounter = 0; structureCounter < numberOfStructures; ++structureCounter) 
        {
            data[structureCounter].candidatesCount = 0;
            data[structureCounter].positionsCount = 0;
        }
        continue;
    }

    fclose(fp);
    printf("Done\n");
    return 0;
}
