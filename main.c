#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void checkArguments (int argc)
{
	if (argc == 1)
	{
		printf("REQUIRED ARGUMENTS:\n~~~~~~~~~~~~~~~~~~~\n\n[*] argv[0] = ./main\n[*] argv[1] = input dump filename\n[*] argv[2] = input dihedral filename.\n\n");
		exit (1);
	}
}

int getNAtoms (FILE *inputDump)
{
	int nAtoms;
	char lineString[2000];

	while (fgets (lineString, 2000, inputDump) != NULL)
	{
		if (strstr (lineString, "ITEM: NUMBER OF ATOMS"))
		{
			fgets (lineString, 2000, inputDump);
			sscanf (lineString, "%d\n", &nAtoms);
			break;
		}
	}

	return nAtoms;
}

int getNDihedrals (FILE *inputDihedral)
{
	int nDihedrals;
	char lineString[2000];

	while (fgets (lineString, 2000, inputDihedral) != NULL)
	{
		if (strstr (lineString, "ITEM: NUMBER OF ENTRIES"))
		{
			fgets (lineString, 2000, inputDihedral);
			sscanf (lineString, "%d\n", &nDihedrals);
			break;
		}
	}

	return nDihedrals;
}

// checkTimestep (FILE *inputFilePointer)
// {
// 	char lineString[2000];
// 	int timestep;

// 	while (fgets (lineString, 2000, inputFilePointer) != NULL)
// 	{
// 		if (strstr (lineString, "ITEM: TIMESTEP"))
// 		{
// 			fgets (lineString, 2000, inputFilePointer);
// 			sscanf (lineString, "%d\n", &timestep);
// 		}
// 	}

// 	return timestep;
// }

char **getDumpDihedralLines (FILE *input, char **dumpDihedralLines, int nEntries, int *timestep, int *defectPresent)
{
	int currentID, previousID = 0, currentLine = 1;

	while (fgets (dumpDihedralLines[0], 2000, input) != NULL)
	{
		if (strstr (dumpDihedralLines[0], "ITEM: TIMESTEP"))
		{
			for (int i = 1; i < 9; ++i)
			{
				fgets (dumpDihedralLines[i], 2000, input);
				if (i == 1) 
					sscanf (dumpDihedralLines[i], "%d\n", &(*timestep));
			}

			for (int i = 0; i < nEntries; ++i)
			{
				fgets (dumpDihedralLines[i + 9], 2000, input);
				sscanf (dumpDihedralLines[i + 9], "%d\n", &currentID);

				if (currentID != (previousID + 1)) 
					(*defectPresent) = 1;

				previousID = currentID;
			}

			return dumpDihedralLines;
		}
	}

}

void printFixedOutput (FILE *inputDump, int nAtoms, FILE *inputDihedral, int nDihedrals, FILE *outputDump, FILE *outputDihedral)
{
	char dumpLine[2000], dihedralLine[2000];
	int timestepDump = 0, timestepDihedral = 0, timestepDump_previous = 0, timestepDihedral_previous = 0, delTime = 0;
	int isDumpFrameDefect = 0, isDihedralFrameDefect = 0;

	char **dumpLines, **dihedralLines;
	printf("Allocating %d * sizeof (char *) mem for dumpLines\n", (nAtoms + 9));
	dumpLines = (char **) malloc ((nAtoms + 9) * sizeof (char *));
	printf("Allocating %d * sizeof (char *) mem for dihedralLines\n", (nDihedrals + 9));
	dihedralLines = (char **) malloc ((nDihedrals + 9) * sizeof (char *));

	for (int i = 0; i < (nAtoms + 9); ++i)
		dumpLines[i] = (char *) malloc (2000 * sizeof (char));

	for (int i = 0; i < (nDihedrals + 9); ++i)
		dihedralLines[i] = (char *) malloc (2000 * sizeof (char));

	do
	{
		timestepDump_previous = timestepDump;
		timestepDihedral_previous = timestepDihedral;

		if ((timestepDump_previous > timestepDump) || (timestepDihedral_previous > timestepDihedral))
		{
			printf("dump ==> %d > %d\n", timestepDump_previous, timestepDump);
			printf("dihedral ==> %d > %d\n", timestepDihedral_previous, timestepDihedral);
			goto endThisCode;
		}

		if ((delTime == 0) && (timestepDump_previous > 0) && (timestepDihedral_previous > 0))
		{
			if ((timestepDump - timestepDump_previous) == (timestepDihedral - timestepDihedral_previous))
			{
				delTime = timestepDump - timestepDump_previous;
			}
		}

		dumpLines = getDumpDihedralLines (inputDump, dumpLines, nAtoms, &timestepDump, &isDumpFrameDefect);
		dihedralLines = getDumpDihedralLines (inputDihedral, dihedralLines, nDihedrals, &timestepDihedral, &isDihedralFrameDefect);

		// If timestepDump == timestepDihedral, then print both dumpLines and dihedralLines
		if (timestepDump == timestepDihedral)
		{
			printf("\rPrinting timeframe (1): %d                         ", timestepDump, timestepDump_previous);
			fflush (stdout);

			if (timestepDump_previous > timestepDump)
			{
				sleep (10);
				goto endThisCode;
			}

			for (int i = 0; i < (nAtoms + 9); ++i) { 
				fprintf(outputDump, "%s", dumpLines[i]); }
			for (int i = 0; i < (nDihedrals + 9); ++i) { 
				fprintf(outputDihedral, "%s", dihedralLines[i]); }
		}

		// If timestepDump < timestepDihedral...
		// Skip timeframes in dump file till timestepDump == timestepDihedral
		else if (timestepDump < timestepDihedral)
		{
			while (timestepDump < timestepDihedral) {
				dumpLines = getDumpDihedralLines (inputDump, dumpLines, nAtoms, &timestepDump, &isDumpFrameDefect); }

			if (timestepDump == timestepDihedral)
			{
				printf("\rPrinting timeframe (2): %d                         ", timestepDump);
				fflush (stdout);

				for (int i = 0; i < (nAtoms + 9); ++i) fprintf(outputDump, "%s", dumpLines[i]);
				for (int i = 0; i < (nDihedrals + 9); ++i) fprintf(outputDihedral, "%s", dihedralLines[i]);
			}
		}

		// Else if timestepDump > timestepDihedral...
		// Skip timeframes in dihedral file till timestepDump == timestepDihedral
		else if (timestepDump > timestepDihedral)
		{
			while (timestepDump > timestepDihedral) {
				dihedralLines = getDumpDihedralLines (inputDihedral, dihedralLines, nDihedrals, &timestepDihedral, &isDihedralFrameDefect); }

			if (timestepDump == timestepDihedral)
			{
				printf("\rPrinting timeframe (3): %d                         ", timestepDump);
				fflush (stdout);

				for (int i = 0; i < (nAtoms + 9); ++i) fprintf(outputDump, "%s", dumpLines[i]);
				for (int i = 0; i < (nDihedrals + 9); ++i) fprintf(outputDihedral, "%s", dihedralLines[i]);
			}
		}

	} while ((timestepDump_previous != timestepDump) || (timestepDihedral_previous != timestepDihedral));

	endThisCode: ;
	printf("\n");

	free (dumpLines);
	free (dihedralLines);
}

int main(int argc, char const *argv[])
{
	checkArguments (argc);

	FILE *inputDump, *inputDihedral, *outputDump, *outputDihedral;

	int isXZ_dump = -1, isXZ_dihedral = -1;
	char *pipeString;
	pipeString = (char *) malloc (500 * sizeof (char));

	// Setting pointer for input dump file
	if (strstr (argv[1], ".xz")) { snprintf (pipeString, 500, "xzcat %s", argv[1]); inputDump = popen (pipeString, "r"); isXZ_dump = 1; }
	else { inputDump = fopen (argv[1], "r"); isXZ_dump = 0; }

	// Setting pointer for input dihedral file
	if (strstr (argv[2], ".xz")) { snprintf (pipeString, 500 , "xzcat %s", argv[2]); inputDihedral = popen (pipeString, "r"); isXZ_dihedral = 1; }
	else { inputDihedral = fopen (argv[2], "r"); isXZ_dihedral = 0; }

	// Adding suffix for output file names
	char *outputDumpName, *outputDihedralName;
	outputDumpName = (char *) malloc (500 * sizeof (char));
	outputDihedralName = (char *) malloc (500 * sizeof (char));
	snprintf (outputDumpName, 200, "%s.fixed", argv[1]);
	snprintf (outputDihedralName, 200, "%s.fixed", argv[2]);

	outputDump = fopen (outputDumpName, "w");
	outputDihedral = fopen (outputDihedralName, "w");

	// Counting number of atoms and dihedral entries
	int nAtoms = getNAtoms (inputDump), nDihedrals = getNDihedrals (inputDihedral);

	// Re-opening the pipe again, because it is not possible to rewind a pipe pointer (like a file pointer)
	if (strstr (argv[1], ".xz")) {
		fclose (inputDump);
		FILE *inputDump;
		snprintf (pipeString, 500, "xzcat %s", argv[1]); 
		inputDump = popen (pipeString, "r"); 
		isXZ_dump = 1; }

	else { rewind (inputDump); }

	if (strstr (argv[2], ".xz")) {
		fclose (inputDihedral);
		FILE *inputDihedral;
		snprintf (pipeString, 500 , "xzcat %s", argv[2]); 
		inputDihedral = popen (pipeString, "r"); 
		isXZ_dihedral = 1; }

	else { rewind (inputDihedral); }

	printFixedOutput (inputDump, nAtoms, inputDihedral, nDihedrals, outputDump, outputDihedral);

	char *xzCompressionString;
	xzCompressionString = (char *) malloc (200 * sizeof (char));

	printf("Compressing fixed dump file...\n");
	snprintf (xzCompressionString, 200, "rm %s.xz", outputDumpName);
	system (xzCompressionString);
	snprintf (xzCompressionString, 200, "xz -ev %s", outputDumpName);
	system (xzCompressionString);

	printf("Compressing fixed dihedral file...\n");
	snprintf (xzCompressionString, 200, "rm %s.xz", outputDihedralName);
	system (xzCompressionString);
	snprintf (xzCompressionString, 200, "xz -ev %s", outputDihedralName);
	system (xzCompressionString);

	free (xzCompressionString);
	free (outputDumpName);
	free (outputDihedralName);
	fclose (inputDump);
	fclose (inputDihedral);
	fclose (outputDump);
	fclose (outputDihedral);
	return 0;
}