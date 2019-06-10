#include <stdio.h>
#include <stdbool.h>
#include "wavfile.h"

int main() {
    WavFile* check = wavFormatOpen("drumLoop.wav");
    wavFormatReadAll(check);


    if ( check ) {
        printf("Sample size: %d\n", check->header->sampleRate);
        printf("File size %ld\n", check->dataLength + sizeof(WavHeader));
    }
    return 0;
}
