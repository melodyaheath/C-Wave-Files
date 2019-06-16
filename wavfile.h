#ifndef _WAVFILE_H_
#define _WAVFILE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* This struct contains data from the "fmt " section of a WavFile. */
typedef struct _WavHeader {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample; 
} WavHeader;

/* A struct for wavFormat* functions. */
typedef struct _WavFile {
    FILE * handle;
    uint64_t dataLength;
    WavHeader * header;
    bool fileIsOpen;
    float * samples;
    bool formatChunkFound;
    bool dataChunkFound;
    size_t sampleCount;
} WavFile;

/*
    Open a *.wav file.
    Returns NULL if the file failed to open, if the "RIFF" & "WAVE" values are missing, or if malloc fails.  
*/
WavFile* wavFormatOpen(const char * filename);
/*
    Reads the "fmt " & "data" sections of a WavFile produced by wavFormatOpen().
    If this call is successful, wavFormatIsReady(WavFile*) will return true.
*/
void wavFormatReadAll(WavFile * hFile);
/*
    Closes a WavFile, freeing all memory owned by the struct.
*/
void wavFormatClose(WavFile * hFile);
/*
    Checks if a WavFile struct was populated successfully by wavFormatReadAll().
*/
bool wavFormatIsReady(WavFile * hFile);
/*
    Takes a WavFile struct and generates a new *.wav file from the contents.
*/
bool wavFormatSave(WavFile * hFile, const char * newFile);
/*
    Gets the current number of samples based on the bitrate and data length.
*/
size_t wavFormatGetSampleCount(WavFile * hFile);
/*
    Get a sample, returns NULL if nothing is read.
*/
float wavFormatGetSample(WavFile * hFile, size_t sample);
/*
    Gets the number of bits per sample. Returns 0 in the case of errors.
*/
uint16_t wavFormatGetBitsPerSample(WavFile * hFile);
/*
    Sets a sample, returns false for errors.
*/
bool wavFormatSetSample(WavFile * hFile, size_t sample, float value);
#endif