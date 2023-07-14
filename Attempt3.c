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
    Pos *candidates;
    int candidatesCount;
    Pos *positions;
    int positionsCount;
} StructData;

const char *FILEPATH = "seeds.txt";
const int MC = MC_1_16_1;
const int STRUCTS[] = {Ruined_Portal};

const uint64_t START_SEED = 0;
// Lowest seed: -9223372036854775808
const uint64_t SEEDS_TO_CHECK = 1000;
// Max seed: 18446744073709551616

int main() 
{
    const int numberOfStructs = sizeof(STRUCTS) / sizeof(*STRUCTS);    
    StructData data[numberOfStructs];
    FILE *fp = fopen(FILEPATH, "a");
    if (!fp) {
        printf("Failed to open file: %s\n", FILEPATH);
        return 1;
    }

    Generator g;
    setupGenerator(&g, MC, 0);
    Pos p;
    Pos spawn;
    uint64_t seed = START_SEED;

    for (int i = 0; i < numberOfStructs; ++i) 
    {
        data[i].candidates = NULL;
        data[i].positions = NULL;
    }

    for (; seed < SEEDS_TO_CHECK; ++seed) 
    {
        applySeed(&g, DIM_OVERWORLD, seed);
        spawn = getSpawn(&g);
        DoublePos origCoords = {{0, 0}, {0, 0}};
        origCoords.first.x = -50 + spawn.x;
        origCoords.first.z = -50 + spawn.z;
        origCoords.second.x = 50 + spawn.x;
        origCoords.second.z = 50 + spawn.z;

        for (int i = 0; i < numberOfStructs; ++i) 
        {
            StructureConfig currentStructureConfig;
            if (!getStructureConfig(STRUCTS[i], MC, &currentStructureConfig)) 
            {
                printf("ERROR: Structure #%d in the STRUCTS array cannot exist in the specified version.\n", i);
                continue;
            }

            data[i].regionCoords.first.x = origCoords.first.x / (currentStructureConfig.regionSize << 4) - (origCoords.first.x < 0);
            data[i].regionCoords.first.z = origCoords.first.z / (currentStructureConfig.regionSize << 4) - (origCoords.first.z < 0);
            data[i].regionCoords.second.x = origCoords.second.x / (currentStructureConfig.regionSize << 4) - (origCoords.second.x < 0);
            data[i].regionCoords.second.z = origCoords.second.z / (currentStructureConfig.regionSize << 4) - (origCoords.second.z < 0);

            data[i].dimension = currentStructureConfig.properties & STRUCT_NETHER ? DIM_NETHER   :
                                currentStructureConfig.properties & STRUCT_END    ? DIM_END      :
                                                                                        DIM_OVERWORLD;

            int regionCountX = data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1;
            int regionCountZ = data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1;

            if (regionCountX <= 0 || regionCountZ <= 0) {
                printf("Invalid region coordinates for Structure #%d\n", i);
                continue;
            }

            data[i].candidates = realloc(data[i].candidates, regionCountX * regionCountZ * sizeof(Pos));
            data[i].positions = realloc(data[i].positions, regionCountX * regionCountZ * sizeof(Pos));

            if (!data[i].candidates || !data[i].positions) {
                printf("Memory allocation failed for Structure #%d\n", i);
                continue;
            }

            data[i].candidatesCount = 0;
            for (int regX = data[i].regionCoords.first.x; regX <= data[i].regionCoords.second.x; ++regX) 
            {
                for (int regZ = data[i].regionCoords.first.z; regZ <= data[i].regionCoords.second.z; ++regZ) 
                {
                    if (!getStructurePos(STRUCTS[i], MC, seed, regX, regZ, &p)) 
                        continue;
                    if ((regX == data[i].regionCoords.first.x  && p.x < origCoords.first.x ) ||
                        (regX == data[i].regionCoords.second.x && p.x > origCoords.second.x) ||
                        (regZ == data[i].regionCoords.first.z  && p.z < origCoords.first.z ) ||
                        (regZ == data[i].regionCoords.second.z && p.z > origCoords.second.z)) 
                        continue;
                    data[i].candidates[data[i].candidatesCount] = p;
                    ++data[i].candidatesCount;
                }
            }

            if (data[i].candidatesCount == 0)
                continue;

            data[i].positionsCount = 0;
            if (g.seed != seed || g.dim != data[i].dimension)
                applySeed(&g, data[i].dimension, seed);
            
            for (int candidate = 0; candidate < data[i].candidatesCount; ++candidate) 
            {
                if (!isViableStructurePos(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z, 0) ||
                    !isViableStructureTerrain(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z)) 
                    continue;
                data[i].positions[data[i].positionsCount].x = data[i].candidates[candidate].x;
                data[i].positions[data[i].positionsCount].z = data[i].candidates[candidate].z;
                ++data[i].positionsCount;
            }

            if (data[i].positionsCount > 0) {
                fprintf(fp, "%" PRId64 "\n", seed);
                break;
            }
        }
    }

    for (int i = 0; i < numberOfStructs; ++i) 
    {
        free(data[i].candidates);
        free(data[i].positions);
    }

    fprintf(fp, "Done\n");
    fclose(fp); 

    return 0;
}
