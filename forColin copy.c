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


const DoublePos origCoords = {{-96, -96}, {96, 96}};
const char *FILEPATH = "seeds.txt";
const int STRUCTS[] = {Bastion, Fortress};
const int MC = MC_1_16_1;
const uint64_t START_STRUCTURE_SEED = 0;
const uint64_t STRUCTURE_SEEDS_TO_CHECK = 1000;
const int UPPER_BITS_TO_CHECK = 1;
//2^16 = 65536, 2^48 = 281474976710656

//return an integer - if 0, then false. if 1, then true. also return x coord as 1 integer and z coord as another
int structureChecker (int lower48, int structure, int MC, DoublePos boxDimensions, StructData data[], int result)
{
	Pos p;
	int i = 0;
	data[i].candidatesCount = 0;
			
	for (int regX = data[i].regionCoords.first.x; regX <= data[i].regionCoords.second.x; ++regX) 
	{
				
		for (int regZ = data[i].regionCoords.first.z; regZ <= data[i].regionCoords.second.z; ++regZ) 
		{
					
			if (!getStructurePos(structure, MC, lower48, regX, regZ, &p)) continue;
					
					
			if ((regX == data[i].regionCoords.first.x  && p.x < origCoords.first.x ) ||
				(regX == data[i].regionCoords.second.x && p.x > origCoords.second.x) ||
				(regZ == data[i].regionCoords.first.z  && p.z < origCoords.first.z ) ||
				(regZ == data[i].regionCoords.second.z && p.z > origCoords.second.z)) continue;
					
					data[i].candidates[data[i].candidatesCount] = p;
					++data[i].candidatesCount;
		}
	}

	if (!data[i].candidatesCount)
	{
		result = 1;
	} 
	else
	{
		result = 2;
	}
	return result;
	
}

int main() 
{


	const int numberOfStructs = sizeof(STRUCTS)/sizeof(*STRUCTS); 
	StructData data[numberOfStructs];				


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
		
		
		data[i].candidates = malloc((data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1) * (data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1) * sizeof(Pos));
		data[i].positions = malloc((data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1) * (data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1) * sizeof(Pos));
	}

	
	FILE *fp = fopen(FILEPATH, "a");
	
	if (!fp) exit(1);

	Generator g;
	setupGenerator(&g, MC, 0);

	int result = 0;

	Pos p;
	


	for (uint64_t lower48 = START_STRUCTURE_SEED; lower48 < STRUCTURE_SEEDS_TO_CHECK; ++lower48) 
	{
	int i = 0;

	DoublePos boxDimensions = {{data[i].regionCoords.first.x, data[i].regionCoords.first.z}, {data[i].regionCoords.second.x, data[i].regionCoords.second.z}};
	result = structureChecker (lower48, Bastion, MC, boxDimensions, data, result);
	if (result == 1) 
	{
    	goto nextStructureSeed;
		//Didn't find a candidate and continuing to the next structure seed
	}
	else
	{
		continue;
	}

	for (uint64_t upper16 = 0; upper16 < UPPER_BITS_TO_CHECK; ++upper16) 
		{
			
			uint64_t seed = lower48 | (upper16 << 48);
			
			for (int i = 0; i < numberOfStructs; ++i) 
			{
				
				data[i].positionsCount = 0;
				
				
				if (g.seed != seed || g.dim != data[i].dimension) applySeed(&g, data[i].dimension, seed);
				
				
				for (int candidate = 0; candidate < data[i].candidatesCount; ++candidate) 
				{
					
					
					if (!isViableStructurePos(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z, 0) ||
					    !isViableStructureTerrain(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z)) continue;
					
					
					data[i].positions[data[i].positionsCount].x = data[i].candidates[candidate].x;
					data[i].positions[data[i].positionsCount].z = data[i].candidates[candidate].z;
					++data[i].positionsCount;
				}
				
				if (!data[i].positionsCount) goto nextStructureSeed;
			}
			
			
			fprintf(fp, "%" PRId64 "\n", seed);
            goto nextStructureSeed;
		}
		
		nextStructureSeed: continue;
	}
	fprintf(fp, "Done\n");
	fclose(fp); 
    return 0;
}  





