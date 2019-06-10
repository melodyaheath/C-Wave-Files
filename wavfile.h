#ifndef _WAVFILE_H_
#define _WAVFILE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct _WavHeader {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample; 
} WavHeader;

typedef struct _WavFile {
    FILE* handle;
    uint64_t dataLength;
    WavHeader* header;
    bool fileIsOpen;
    unsigned char * samples;
    bool formatChunkFound;
    bool dataChunkFound;
} WavFile;


WavFile* wavFormatOpen(const char *filename);
void wavFormatReadAll(WavFile * hFile);
void wavFormatClose(WavFile *hFile);
bool wavFormatIsReady(WavFile * hFile);
#endif