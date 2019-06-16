#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "wavfile.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage\n%s [src] [dest]\n", argv[0]);
        printf("[src]  - source wav file\n");
        printf("[dest] - destination wav file\n");
        return 0;
    } 
    printf("Loading: %s\n", argv[1]);
    WavFile* source = wavFormatOpen(argv[1]);
    if (source == NULL) {
        fprintf(stderr,"Failed to open %s\n", argv[1]);
        return -1;
    }
    printf("Processing format and data chunks\n");
    wavFormatReadAll(source);
    
    if (!wavFormatIsReady(source)) {
        fprintf(stderr,"Failed to process %s\n", argv[1]);
        wavFormatClose(source);
        return -1;
    }

    int sampleCount = wavFormatGetSampleCount(source);
    printf("Number of samples loaded: %d\n", sampleCount);
    printf("Bits per sample: %hu\n", wavFormatGetBitsPerSample(source));

    printf("Changing samples\n");
    for ( size_t i = 0; i < sampleCount; i++ ) {
        float sample = wavFormatGetSample(source, i);
        if (!wavFormatSetSample(source, i, sample)) {
            fprintf(stderr, "Failed to update sample\n");
            wavFormatClose(source);
            return -1;
        }
    }
    bool destination = wavFormatSave(source, argv[2]);
    if (!destination) {
        fprintf(stderr,"Failed to save changes to %s\n", argv[2]);
        wavFormatClose(source);
        return -1;
    }

    wavFormatClose(source);
    printf("Successfully generated %s\n", argv[2]);
    return 0;
}
