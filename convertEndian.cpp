#include <cstdio>

int main(int argc, char** argv) {
    if (argc == 1) {
        fprintf(stderr, "No input file specified\n");
        return 1;
    }
    if (argc == 2) {
        fprintf(stderr, "No output file specified\n");
        return 1;
    }
    if (argc > 3) {
        fprintf(stderr, "Extraneous arguments\n");
    }
    char* inFile = argv[1];
    char* outFile = argv[2];

    FILE* in = NULL;
    if ((inFile[0] == '-') && (inFile[1] == 0))
        in = stdin;
    else
        in = fopen(inFile, "rb");
    if (in == NULL) {
        fprintf(stderr, "Failed to open input file \"%s\"\n", inFile);
        return 2;
    }

    FILE* out = NULL;
    if ((outFile[0] == '-') && (outFile[1] == 0))
        out = stdout;
    else
        out = fopen(outFile, "wb");
    if (out == NULL) {
        fprintf(stderr, "Failed to open output file \"%s\"\n", outFile);
        fclose(in);
        return 2;
    }

    while (1==1) {
        int c1 = fgetc(in);
        if (c1 == EOF)
            break;
        int c2 = fgetc(in);
        if (c2 == EOF)
            c2 = 0;
        fputc(c2, out);
        fputc(c1, out);
    }

    fclose(in);
    fclose(out);
    return 0;
}
