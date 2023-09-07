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
    int i = structureIndex; // Use the correct data element corresponding to the structure being checked
    data[i].candidatesCount = 0;
    int currentStructure = STRUCTS[structureIndex];

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
	int gateway_x = (int)(96.0 * cos(angle));
    int gateway_z = (int)(96.0 * sin(angle));
	Pos result;
	result.x = gateway_x;
	result.z = gateway_z;

    return result;
}

Pos linkedGateway(uint64_t lower48)
{
	uint64_t rng = 0;               // some initial value idk
	setSeed(&rng, lower48);            // new Random(seed);
	int rngResult = nextInt(&rng, 20); // nextInt(20);
	double angle = 2.0 * (-1 * PI + 0.15707963267948966 * (rngResult));
	int gateway_x = (int)(1024.0 * cos(angle));
    int gateway_z = (int)(1024.0 * sin(angle));
	Pos result;
	result.x = gateway_x;
	result.z = gateway_z;

    return result;
}

Pos goodMainIslandGateway[12] = 
{
	{ 96,  0}, { 91, 29}, { 29, 91}, {  0, 96},
	{-29, 91}, {-91, 29}, {-96,  0}, {-91,-29},
	{-29,-91}, {  0,-96}, { 29,-91}, { 91,-29},
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

    uint64_t START_STRUCTURE_SEED, STRUCTURE_SEEDS_TO_CHECK;
    if (fscanf(fp, "%" SCNu64 "%" SCNu64, &START_STRUCTURE_SEED, &STRUCTURE_SEEDS_TO_CHECK) != 2) 
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

    for (int i = 0; i < numberOfStructs; ++i) 
    {
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
    }

    Generator g;
    setupGenerator(&g, MC, 0);

    int biome = 0;
    int result = 0;
    uint64_t seed;

    for (uint64_t lower48 = START_STRUCTURE_SEED; lower48 < STRUCTURE_SEEDS_TO_CHECK; ++lower48) 
    {
        DoublePos bastionBoundingBox = {{-96, -96}, {96, 96}};

        result = structureChecker(lower48, STRUCTS, 0, MC, bastionBoundingBox, data, result, bastionCoordinates, fortressCoordinates, endCityCoordinates);
        if (result == 1) 
        {
            continue;
        } 
        else 
        {
            for (int bastionIdx = 0; bastionIdx < data[0].candidatesCount; ++bastionIdx) 
            {
                DoublePos fortressBoundingBox = {{-96 + bastionCoordinates[bastionIdx].x, -96 + bastionCoordinates[bastionIdx].z}, {96 + bastionCoordinates[bastionIdx].x, 96 + bastionCoordinates[bastionIdx].z}};
                result = structureChecker(lower48, STRUCTS, 1, MC, fortressBoundingBox, data, result, bastionCoordinates, fortressCoordinates, endCityCoordinates);
                if (result == 1) 
                {
                    continue;
                } 
                else 
                {
                    int allChecksFailed = 1;

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

		            Pos outerGateway;
		            outerGateway = linkedGateway(lower48);
                    origCoords = (DoublePos) {{-96 + outerGateway.x, -96 + outerGateway.z}, {96 + outerGateway.x, 96 + outerGateway.z}};

                    for (int i = 2; i < 3; ++i) 
                    {
                        StructureConfig currentStructureConfig; 
                        
                        if (!getStructureConfig(STRUCTS[i], MC, &currentStructureConfig)) 
                        {
                            printf("ERROR: Structure #%d in the STRUCTS array cannot exist in the specified version.\n", i);
                            exit(1);
                        }
                        
                        switch (currentStructureConfig.regionSize) {
                        case 32:
                            data[i].regionCoords.first.x  = origCoords.first.x  >> 9;
                            data[i].regionCoords.first.z  = origCoords.first.z  >> 9;
                            data[i].regionCoords.second.x = origCoords.second.x >> 9;
                            data[i].regionCoords.second.z = origCoords.second.z >> 9;
                            break;
                        case 1:
                            data[i].regionCoords.first.x  = origCoords.first.x  >> 4;
                            data[i].regionCoords.first.z  = origCoords.first.z  >> 4;
                            data[i].regionCoords.second.x = origCoords.second.x >> 4;
                            data[i].regionCoords.second.z = origCoords.second.z >> 4;
                            break;
                        default:
                            data[i].regionCoords.first.x  = (origCoords.first.x  / (currentStructureConfig.regionSize << 4)) - (origCoords.first.x  < 0);
                            data[i].regionCoords.first.z  = (origCoords.first.z  / (currentStructureConfig.regionSize << 4)) - (origCoords.first.z  < 0);
                            data[i].regionCoords.second.x = (origCoords.second.x / (currentStructureConfig.regionSize << 4)) - (origCoords.second.x < 0);
                            data[i].regionCoords.second.z = (origCoords.second.z / (currentStructureConfig.regionSize << 4)) - (origCoords.second.z < 0);
                            break;
                        }
                        
                        data[i].dimension = currentStructureConfig.properties & STRUCT_NETHER ? DIM_NETHER   :
                                            currentStructureConfig.properties & STRUCT_END    ? DIM_END      :
                                                                                                DIM_OVERWORLD;
                    }
                    
                    result = structureChecker(lower48, STRUCTS, 2, MC, origCoords, data, result, bastionCoordinates, fortressCoordinates, endCityCoordinates);
                    if (result == 1) 
                    {
                        continue;
                    }
                    else 
                    {
                        int lastIndex = data[i].candidatesCount - 1;
                        int lastX = endCityCoordinates[lastIndex].x;
                        int lastZ = endCityCoordinates[lastIndex].z;

                        SurfaceNoise sn;
                        initSurfaceNoise(&sn, 1 /* World dimension */, lower48);

                        if (isViableEndCityTerrain(&g, &sn, lastX, lastZ)) continue;

                        int deltaX = outerGateway.x - lastX;
                        int deltaZ = outerGateway.z - lastZ;
                        int distanceSquared = deltaX * deltaX + deltaZ * deltaZ;
                        int maxDistanceSquared = 96 * 96;
                        if (distanceSquared <= maxDistanceSquared) 
                        {
                        } 
                        else 
                        {
                            goto nextStructureSeed;
                        }
                        
                        int allChecksFailed = 1;
                        
                        for (uint64_t upper16 = 0; upper16 < UPPER_BITS_TO_CHECK; ++upper16) 
                        {
                            seed = lower48 | (upper16 << 48);
                            for (int i = 0; i < numberOfStructs; ++i) 
                            {
                                data[i].positionsCount = 0;
                                if (g.seed != seed || g.dim != data[i].dimension) applySeed(&g, data[i].dimension, seed);
                                for (int j = 0; j < data[i].candidatesCount; ++j) 
                                {
                                    if (!isViableStructurePos(STRUCTS[i], &g, data[i].candidates[j].x, data[i].candidates[j].z, 0) ||
                                        !isViableStructureTerrain(STRUCTS[i], &g, data[i].candidates[j].x, data[i].candidates[j].z))
                                        continue;
                                    data[i].positions[data[i].positionsCount] = data[i].candidates[j];
                                    data[i].positionsCount++;
                                }
                                if (data[i].positionsCount > 0) 
                                {
                                    allChecksFailed = 0;
                                    break;
                                }
                            }
                            if (!allChecksFailed) 
                                break;
                        }

                        if (allChecksFailed) 
                            continue;

                        biome = getBiomeAt(&g, 1, bastionCoordinates[bastionIdx].x, 64, bastionCoordinates[bastionIdx].z);
                        if (biome == basalt_deltas) 
                        {
                            goto nextStructureSeed;
                        }

                        biome = getBiomeAt(&g, 1, fortressCoordinates[bastionIdx].x, 64, fortressCoordinates[bastionIdx].z); 
                        if (biome != soul_sand_valley) 
                        {    
                            goto nextStructureSeed;
                        }
                        printf("%" PRId64 "\n", seed);
                        //printf("%d %d\n", innerGateway.x, innerGateway.z);
                        //printf("%d %d\n", outerGateway.x, outerGateway.z);
                        //printf("%d %d %d %d\n", endCityBoundingBox.first.x, endCityBoundingBox.first.z, endCityBoundingBox.second.x, endCityBoundingBox.second.z);
                        goto nextStructureSeed;
                    }
                }
            }
        }

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
