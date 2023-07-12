// Imports Cubiomes files (currently assumes all are in a folder called "cubiomes"); you can change it to your liking
#include "finders.c"
#include "generator.c"
#include "biome_tree.c"
#include "layers.c"
#include "noise.c"

// Since I'll be working with two sets of coordinates a lot, I made things easier for myself by defining a structure named DoublePos consisting of two Poses.
// The exact same functionality could be achieved with two Poses each time, or four sets of ints each time, but I thought this was the least unwieldy.
typedef struct {
	Pos first, second;
} DoublePos;

// I also created a structure to store the data that'll be associated with each structure during our search:
typedef struct {
	DoublePos regionCoords;	// Stores the structure's region's bounding coordinates
	int dimension;			// Stores the structure's dimension
	Pos *candidates;
	int candidatesCount;
	Pos *positions;			// Stores the list of positions for the structure within each region
	int positionsCount;		// Stores the number of positions actually stored in the positions array
} StructData;
// --------------------------------------------------------------------------------------------------------------------

// To make maintaining this easier in the future (or if I want to at some point spread this across multiple files), I put all of our settings for this program here, in global scope, as constant variables.
// The coords to search within
DoublePos origCoords = {{-300, -300}, {300, 300}};
// The output filepath
const char *FILEPATH = "seeds.txt";
// The structures; to handle multiple of them, I turned that into an array.
const int STRUCTS[] = {Ruined_Portal};
// The version
const int MC = MC_1_16_1;
// The structure seed to start at
const uint64_t START_STRUCTURE_SEED = 0;
// The number of structure seeds to check (max 2^48 = 281474976710656) (2^32 = 4294967296)
const uint64_t STRUCTURE_SEEDS_TO_CHECK = 65536;
// The number of actual seeds to check once a potential structure seed is found (max 2^16 = 65536)
const int UPPER_BITS_TO_CHECK = 1;
// Getting spawn
    Pos spawn;
// --------------------------------------------------------------------------------------------------------------------

int main() {
	// First off, each structure has a particular-size region inside which it can only generate once. So we need to convert our coordinates above into the equivalent coordinates of the regions, which we can then iterate over.
	// We begin by getting the region size for each structure.

	const int numberOfStructs = sizeof(STRUCTS)/sizeof(*STRUCTS); // The number of structures in our array; has the benefit of automatically updating			
	StructData data[numberOfStructs];				
	// _Bool endDetected = 0;

	// Now we open the output file and set up the generator, as in the original program.
	FILE *fp = fopen(FILEPATH, "a");
	// It's usually a good idea to make sure fopen worked by checking if fp is a null pointer, and exiting if so (since otherwise you'll get a segmentation fault when you try to write to the file)
	if (!fp) exit(1);

	Generator g;
	setupGenerator(&g, MC, 0);
	// SurfaceNoise sn;
	// EndNoise en;

	// Now we go through two iterations. First, we check the lowest 48 bits alone and see if our structure seed could even give us all of our desired structures inside of our bounding box. If it can't, we continue on; otherwise, we proceed to the second iteration, where we see whether each full seed actually gives us those structures in those candidate positions or not. If not, we continue on; otherwise, we print the seed and the coordinates of each structure.
	Pos p;
	// I moved the lower48 definition inside the for loop, since it's not invoked at all outside of it; that's mostly just personal preference, though
for (uint64_t lower48 = START_STRUCTURE_SEED; lower48 < STRUCTURE_SEEDS_TO_CHECK; ++lower48) {
		// Here we iterate over each entry in our structTypes array.
        
        spawn = getSpawn(&g);
        DoublePos origCoords = {{0, 0}, {0, 0}};
        origCoords = (DoublePos) {{-300 + spawn.x, -300 + spawn.z}, {300 + spawn.x, 300 + spawn.z}};











    const int numberOfStructs = sizeof(STRUCTS)/sizeof(*STRUCTS); // The number of structures in our array; has the benefit of automatically updating			
	StructData data[numberOfStructs];				
	// _Bool endDetected = 0;

	for (int i = 0; i < numberOfStructs; ++i) {
		StructureConfig currentStructureConfig; // To temporarily store each structure configuration in
		// Assigns the current struct's configuration into currentStructureConfig. Per the Cubiomes documentation, this returns 0 if the structure can't exist in that version, in which case...
		if (!getStructureConfig(STRUCTS[i], MC, &currentStructureConfig)) {
			// ...we print an error message and abort the program.
			printf("ERROR: Structure #%d in the STRUCTS array cannot exist in the specified version.\n", i);
			exit(1);
		}

		// Otherwise we use the region size to convert the coordinates.
		// This is copied straight from Cubiomes Viewer's source code, so don't ask me to explain the reasoning behind it.
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

		// Sets the current struct's dimension in structDimensions using its config.properties value
		data[i].dimension = currentStructureConfig.properties & STRUCT_NETHER ? DIM_NETHER   :
		                    currentStructureConfig.properties & STRUCT_END    ? DIM_END      :
		                                                                        DIM_OVERWORLD;
		// if (data[i].dimension == DIM_END) endDetected = 1;
		
		// Then allocates the memory for the candidates and positions arrays.
		// Since the number of regions we have is (second.x - first.x + 1) * (second.z - first.z + 1), we merely need to multiply that by the size of a Pos struct, or sizeof(Pos).
		data[i].candidates = malloc((data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1) * (data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1) * sizeof(Pos));
		data[i].positions = malloc((data[i].regionCoords.second.x - data[i].regionCoords.first.x + 1) * (data[i].regionCoords.second.z - data[i].regionCoords.first.z + 1) * sizeof(Pos));























		for (int i = 0; i < numberOfStructs; ++i) {
			// Reset the number of candidates we've found to 0
			data[i].candidatesCount = 0;
			// Then we iterate over each region coordinate along the x-axis...
			for (int regX = data[i].regionCoords.first.x; regX <= data[i].regionCoords.second.x; ++regX) {
				// Then we iterate over each region coordinate along the z-axis...
				// (Yes, this has O(n^4) complexity; no, this does not scale well in the slightest)
				for (int regZ = data[i].regionCoords.first.z; regZ <= data[i].regionCoords.second.z; ++regZ) {
					// First we get the theoretical structure position for that region. If the function can't even find the theoretical position, continue to the next region.
					if (!getStructurePos(STRUCTS[i], MC, lower48, regX, regZ, &p)) continue;
					// Otherwise make sure that position's within our original bounds.
					// (The only time it couldn't be would be if one of our region coordinates is on the outer border, hence the first set of checks.)
					if ((regX == data[i].regionCoords.first.x  && p.x < origCoords.first.x ) ||
					    (regX == data[i].regionCoords.second.x && p.x > origCoords.second.x) ||
						(regZ == data[i].regionCoords.first.z  && p.z < origCoords.first.z ) ||
					    (regZ == data[i].regionCoords.second.z && p.z > origCoords.second.z)) continue;
					// Otherwise add the position to our candidates array, and increment our candidates counter
					data[i].candidates[data[i].candidatesCount] = p;
					++data[i].candidatesCount;
				}
			}
			// If the counter is still 0 at the end of that, we didn't find any candidates for that structure; continue to the next structure seed
			if (!data[i].candidatesCount) goto nextStructureSeed;
		}

		// Otherwise, start incrementing over the possible upper bits.
		for (uint64_t upper16 = 0; upper16 < UPPER_BITS_TO_CHECK; ++upper16) {
			// Create full seed
			uint64_t seed = lower48 | (upper16 << 48);
			// Iterate over structures list again
			for (int i = 0; i < numberOfStructs; ++i) {
				// Reset number of actual positions we've found to 0
				data[i].positionsCount = 0;
				// If the generator's seed or dimension is inaccurate, run applySeed to update it. This reduces the number of times we need to call this as much as possible, since this is a very expesnive function in 1.18+.
				// (We could in theory reduce it more by preemptively sorting the structure list by dimension, but I'm not going to implement that.)
				if (g.seed != seed || g.dim != data[i].dimension) applySeed(&g, data[i].dimension, seed);
				// if (g.dim == DIM_END && sn.)
				// Iterate over all candidate positions for the structure
				for (int candidate = 0; candidate < data[i].candidatesCount; ++candidate) {
					// If the biomes or terrain don't allow for it, continue
					// TODO: Implement End City check
					if (!isViableStructurePos(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z, 0) ||
					    !isViableStructureTerrain(STRUCTS[i], &g, data[i].candidates[candidate].x, data[i].candidates[candidate].z)) continue;
					// if (STRUCTS[i] == End_City && !isViableEndCityTerrain(&en, &sn, data[i].candidates[candidate].x, data[i].candidates[candidate].z)) continue;
					// Otherwise, add candidate position as actual position, and increment positions counter
					data[i].positions[data[i].positionsCount].x = data[i].candidates[candidate].x;
					data[i].positions[data[i].positionsCount].z = data[i].candidates[candidate].z;
					++data[i].positionsCount;
				}
				// If the counter is still 0 at the end of that, we didn't find any actual positions for that structure; continue to the next full seed
				if (!data[i].positionsCount) goto nextSeed;
			}
			// Then print the seed and the coordinates of each structure.
			// TODO: Maybe rework thisto be thread-safe
			fprintf(fp, "%" PRId64 "\n", seed); // I would *really* recommend PRId64/PRIu64. If you really don't want to use that, I'd heavily recommend using %ll or %llu instead (not %lu), since that'll maximize your chances of other platforms actually being able to print the correct seed. (E.g. on my Windows computer, long is only 32 bits, so if you used %lu and I ran your program all of the seeds would be truncated to 32 bits, which isn't even enough to print every structure seed correctly--much less every actual full seed.)
			// Note from Colin: I set the next 5 lines to just comments, as I don't want to straight up delete the code if it's useful later but at the moment I just want the seeds (cheaper to compute/store and easier to filter later)
			// for (int i = 0; i < numberOfStructs; ++i) {
			//	fprintf(fp, "%d: ", STRUCTS[i]); // Structure index
			//	for (int j = 0; j < data[i].positionsCount; ++j) fprintf(fp, "(%d %d) ", data[i].positions[j].x, data[i].positions[j].z); //Structure positions
			//	fprintf(fp, "\n");
			// }
			// If we jump to here, continue to next full seed.
			nextSeed: continue;
		}
		// If we jump to here, continue to next structure seed.
		nextStructureSeed: continue;
	}
	// Then free all of the memory spaces we allocated earlier.
	for (int i = 0; i < numberOfStructs; ++i) {
		free(data[i].candidates);
		free(data[i].positions);
	}
	// Close the file pointer and end the program.
    fprintf(fp, "Done\n");
	fclose(fp);
	return 0;
}  
}