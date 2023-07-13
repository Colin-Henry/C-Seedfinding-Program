#include "finders.c"
#include "generator.c"
#include "biome_tree.c"
#include "layers.c"
#include "noise.c"

typedef struct {
	Pos first, second;
} DoublePos;

typedef struct {
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


DoublePos origCoords = {{-300, -300}, {300, 300}};
const uint64_t START_SEED = 0;
//Lowest seed: -9223372036854775808
const uint64_t SEEDS_TO_CHECK = 0xffff;
//Max seed: 18446744073709551616

int main() {
	const int numberOfStructs = sizeof(STRUCTS)/sizeof(*STRUCTS);		
	StructData data[numberOfStructs];				
	FILE *fp = fopen(FILEPATH, "a");
	if (!fp) exit(1);

	Generator g;
	setupGenerator(&g, MC, 0);
    Pos p;
    Pos spawn;
    uint64_t seed = START_SEED;

    malloc (8589934592);

for (uint64_t START_SEED; seed < SEEDS_TO_CHECK; ++seed) {
    spawn = getSpawn(&g);
    DoublePos origCoords = {{0, 0}, {0, 0}};
    origCoords = (DoublePos) {{-300 + spawn.x, -300 + spawn.z}, {300 + spawn.x, 300 + spawn.z}};
    applySeed(&g, DIM_OVERWORLD, seed);

    const int numberOfStructs = sizeof(STRUCTS)/sizeof(*STRUCTS);		
	StructData data[numberOfStructs];				

	for (int i = 0; i < numberOfStructs; ++i) {
		StructureConfig currentStructureConfig;
		if (!getStructureConfig(STRUCTS[i], MC, &currentStructureConfig)) {
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

		data[i].candidates = realloc(data[i].candidates, (data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1) * (data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1) * sizeof(Pos));
		data[i].positions = realloc(data[i].positions, (data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1) * (data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1) * sizeof(Pos));

		for (int i = 0; i < numberOfStructs; ++i) {
			data[i].candidatesCount = 0;
			for (int regX = data[i].regionCoords.first.x; regX <= data[i].regionCoords.second.x; ++regX) {
				for (int regZ = data[i].regionCoords.first.z; regZ <= data[i].regionCoords.second.z; ++regZ) {
					if (!getStructurePos(STRUCTS[i], MC, seed, regX, regZ, &p)) continue;
					if ((regX == data[i].regionCoords.first.x  && p.x < origCoords.first.x ) ||
					    (regX == data[i].regionCoords.second.x && p.x > origCoords.second.x) ||
						(regZ == data[i].regionCoords.first.z  && p.z < origCoords.first.z ) ||
					    (regZ == data[i].regionCoords.second.z && p.z > origCoords.second.z)) continue;
					data[i].candidates[data[i].candidatesCount] = p;
					++data[i].candidatesCount;
				}
			}
			if (!data[i].candidatesCount) goto nextSeed;
		}

        for (int i = 0; i < numberOfStructs; ++i) {
				data[i].positionsCount = 0;
				if (g.seed != seed || g.dim != data[i].dimension) applySeed(&g, data[i].dimension, seed);
				for (int candidate = 0; candidate < data[i].candidatesCount; ++candidate) {
					// TODO: Implement End City check
					if (!isViableStructurePos(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z, 0) ||
					    !isViableStructureTerrain(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z)) continue;
					data[i].positions[data[i].positionsCount].x = data[i].candidates[candidate].x;
					data[i].positions[data[i].positionsCount].z = data[i].candidates[candidate].z;
					++data[i].positionsCount;
				}
				if (!data[i].positionsCount) goto nextSeed;
			}
			fprintf(fp, "%" PRId64 "\n", seed);
        nextSeed: continue;
    }
    for (int i = 0; i < numberOfStructs; ++i) {
		free(data[i].candidates);
		free(data[i].positions);
	}
    fprintf(fp, "Done\n");
	fclose(fp);
	return 0;
}
}