#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "finders.c"
#include "generator.c"
#include "biomenoise.c"
#include "biometree.c"
#include "noise.c"
#include "layers.c"

typedef struct 
{
    Pos first, second;
} DoublePos;

typedef struct 
{
    DoublePos regionCoords;
    int dimension;
    Pos candidates[4];
    Pos positions[4];
    int candidatesCount;
    int positionsCount;
} StructData;

DoublePos origCoords = {{-96, -96}, {96, 96}};
DoublePos bastionBoundingBox = {{-96, -96}, {96, 96}};
int structs[] = {Bastion, Fortress};
const int MC = MC_1_16_1;
const int upperBits = 1;
const int numberOfStructs = sizeof(structs) / sizeof(*structs);
int structureIndex = 0;
int biome = 0;
bool result = false;

bool structureChecker(int lower48, int structs[], int structureIndex, int MC, DoublePos origCoords, StructData data[], bool result, Pos* structureCoordinates) 
{
    for (int i = 0; i < numberOfStructs; ++i) 
    {
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
    }

    Pos p;
    int i = structureIndex; // Use the correct data element corresponding to the structure being checked
    data[i].candidatesCount = 0;
    int currentStructure = structs[structureIndex];

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
        result = false;
    } 
    else if (structureIndex == 0) 
    {
        result = true;
        for (int j = 0; j < data[i].candidatesCount; ++j) 
        {
            structureCoordinates[j].x = data[i].candidates[j].x;
            structureCoordinates[j].z = data[i].candidates[j].z;
        }
    } 
    else if (structureIndex == 1) 
    {
        result = true;
        for (int j = 0; j < data[i].candidatesCount; ++j) 
        {
            structureCoordinates[j].x = data[i].candidates[j].x;
            structureCoordinates[j].z = data[i].candidates[j].z;
        }
    }
    return result;
}

int main(int argc, char **argv) 
{
    FILE *fp;
    char *inputPath = "D:/Seedfinding/test/seedRange.txt";

    FILE *file;
    const char *filename = "D:/Seedfinding/test/seeds.txt";
    file = fopen(filename, "a");
    
    if (file == NULL) 
    {
        printf("Error opening file!\n");
        return 1; // Return an error code
    }

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
    
    StructData data[numberOfStructs];

    int i = 0;
    data[i].candidatesCount = 0;
    data[i].positionsCount = 0;

    Pos bastionCoordinates[4];
    Pos fortressCoordinates[4];

    Generator g;
    setupGenerator(&g, MC, 0);
    
    uint64_t seed;

    for (uint64_t lower48 = startingStructureSeed; lower48 < endingStructureSeed; ++lower48) 
    {
        bool checkFailed = true;

        result = structureChecker(lower48, structs, 0, MC, bastionBoundingBox, data, result, bastionCoordinates);
        if (result == false) continue; // If result is true, structure generated. If false, structure did not generate in bounding box

        for (int bastionInstance = 0; bastionInstance < data[0].candidatesCount; ++bastionInstance) 
        {
            DoublePos fortressBoundingBox = {{-96 + bastionCoordinates[bastionInstance].x, -96 + bastionCoordinates[bastionInstance].z}, {96 + bastionCoordinates[bastionInstance].x, 96 + bastionCoordinates[bastionInstance].z}};
            result = structureChecker(lower48, structs, 1, MC, fortressBoundingBox, data, result, fortressCoordinates);
            if (result == false) continue; // If result is true, structure generated. If false, structure did not generate in bounding box
            
            for (uint64_t upper16 = 0; upper16 < upperBits; ++upper16) 
            {
                seed = lower48 | (upper16 << 48);
                for (int i = 0; i < numberOfStructs; ++i) 
                {
                    data[i].positionsCount = 0;
                    if (g.seed != seed || g.dim != data[i].dimension) applySeed(&g, data[i].dimension, seed);
                    for (int j = 0; j < data[i].candidatesCount; ++j) 
                    {
                        if (!isViableStructurePos(structs[i], &g, data[i].candidates[j].x, data[i].candidates[j].z, 0) ||
                            !isViableStructureTerrain(structs[i], &g, data[i].candidates[j].x, data[i].candidates[j].z))
                            continue;
                        data[i].positions[data[i].positionsCount] = data[i].candidates[j];
                        data[i].positionsCount++;
                    }
                    if (data[i].positionsCount > 0) 
                    {
                        checkFailed = false;
                        break;
                    }
                }
                if (!checkFailed) break;
            }

            if (checkFailed) continue;

            biome = getBiomeAt(&g, 1, bastionCoordinates[bastionInstance].x, 64, bastionCoordinates[bastionInstance].z);
            if (biome == basalt_deltas)
            {
                checkFailed = true;
                continue;
            }

            biome = getBiomeAt(&g, 1, fortressCoordinates[bastionInstance].x, 64, fortressCoordinates[bastionInstance].z); 
            if (biome != soul_sand_valley)
            {
                checkFailed = true;
                continue;
            }
        }

        if (checkFailed) continue; // If any of the checks have failed (fastion and nether biomes) up to this point, go to the next seed

        fprintf(file, "%" PRId64 "\n", seed);

        for (int i = 0; i < numberOfStructs; ++i) 
        {
            data[i].candidatesCount = 0;
            data[i].positionsCount = 0;
        }
        continue;
    }

    fprintf(file, "Done\n");
    fclose(fp);
    fclose(file);
    return 0;
}
