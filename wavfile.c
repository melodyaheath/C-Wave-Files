#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "wavfile.h"

bool wavFormatIsProcessable(WavFile * hFile);
bool wavFormatProcessNextChunk(WavFile * hFile);
bool wavFormatProcessDescriptorChunk(WavFile * hFile);
bool wavFormatProcessDataChunk(WavFile * hFile);
bool wavFormatProcessFormatChunk(WavFile * hFile);
bool wavFormatProcessUnknownChunk(WavFile*  hFile);

// Map values to the range of  [-1.F,  1.F]
const float U8_CONVERSION_FACTOR = 2.F/UINT8_MAX;
const float I16_CONVERSION_FACTOR = 2.F/INT16_MAX;
const float I32_CONVERSION_FACTOR = 2.F/INT32_MAX;

float u8ToSample(uint8_t sample) {
    return sample * U8_CONVERSION_FACTOR - 1.F;
}
float i16ToSample(int16_t sample) {
    return sample * I16_CONVERSION_FACTOR;
}
float i32ToSample(int32_t sample) {
    return sample * I32_CONVERSION_FACTOR;
}

bool wavFormatIsProcessable(WavFile * hFile) {
    return hFile != NULL && hFile->fileIsOpen && !feof( hFile->handle );
}

bool wavFormatProcessNextChunk(WavFile * hFile) {
    if ( !wavFormatIsProcessable(hFile) ) {
        return false; 
    }

    char buffer[4];
    size_t bytesRead = fread(&buffer, 1, 4, hFile->handle);
    if (bytesRead != 4) {
        // If we can't grab a tag, we are at the end of the file.
        return false;
    }

    if (strcmp(buffer, "data") == 0) {
        return wavFormatProcessDataChunk(hFile);
    }
    else if (strcmp(buffer, "fmt ") == 0) {
        return wavFormatProcessFormatChunk(hFile);
    }
    return wavFormatProcessUnknownChunk(hFile);
}

bool wavFormatProcessDataChunk(WavFile * hFile) {
   if (!wavFormatIsProcessable(hFile)) {
        return false; 
    }

    size_t bytesRead = fread(&(hFile->dataLength), 1, 4, hFile->handle);
    size_t totalBytesRead = 0;
    if (bytesRead != 4) {
        return false;
    } 

    hFile->samples = malloc(wavFormatGetSampleCount(hFile) * sizeof(float));
    if (hFile->samples == NULL) { 
        return false;
    }

    size_t i = 0;
    size_t expectedSize =  hFile->header->bitsPerSample / 8;
    bytesRead = hFile->header->bitsPerSample / 8;
    
    while ( totalBytesRead < hFile->dataLength && bytesRead == expectedSize){
        if ( expectedSize == 1 ) {
            uint8_t sample = 0;
            bytesRead = fread(&sample, 1, expectedSize, hFile->handle);

            if (bytesRead != expectedSize) {
                return false;
            }
            hFile->samples[i++] = u8ToSample(sample);
        }
        else if ( expectedSize == 2 ) {
            int16_t sample = 0;
            bytesRead = fread(&sample, 1, expectedSize, hFile->handle);

            if (bytesRead != expectedSize) {
                return false;
            }
            hFile->samples[i++] = i16ToSample(sample);
        }
        else if ( expectedSize == 4) {
            int32_t sample = 0;
            bytesRead = fread(&sample, 1, expectedSize, hFile->handle);

            if (bytesRead != expectedSize) {
                return false;
            }
            hFile->samples[i++] = i32ToSample(sample);
        }
        else {
            // We don't handle any variants outside of 1, 2, and 4 byte samples
            return false;
        }
        totalBytesRead += bytesRead;
    }
    

    hFile->dataChunkFound = true;
    return true;
}

bool wavFormatProcessFormatChunk(WavFile * hFile) {
   if (!wavFormatIsProcessable(hFile)) {
        return false; 
    }

    size_t pcmCheck = 0;
    size_t bytesRead = fread(&(pcmCheck), 1, 4, hFile->handle);
    if (bytesRead != 4 || pcmCheck != 16) {
        return false;
    } 

    hFile->header = malloc(sizeof(WavHeader));
    if (hFile->header == NULL) {
        return false;
    }

    size_t result = fread(hFile->header, sizeof(WavHeader), 1, hFile->handle);
    if (result != 1) {
        return false;
    }

    hFile->formatChunkFound = true;
    return true;
}

bool wavFormatProcessUnknownChunk(WavFile * hFile) {
   if (!wavFormatIsProcessable(hFile)) {
        return false; 
    }

    size_t seek = 0;
    size_t bytesRead = fread(&(seek), 1, 4, hFile->handle);
    if (bytesRead != 4) {
        return false;
    } 

    fseek(hFile->handle, seek, SEEK_CUR);

    return !feof(hFile->handle);
}

void wavFormatClose(WavFile * hFile) {
    if (hFile  != NULL) {
        if (hFile->fileIsOpen) {
            fclose(hFile->handle);
        }
        if (hFile->header != NULL) {
            free(hFile->header);
        }
        if (hFile->samples != NULL) {
            free(hFile->samples);
        }
        free(hFile);
    }
}


WavFile * wavFormatOpen(const char * filename) {
    WavFile * hFile = malloc(sizeof(WavFile));
    if (!hFile) {
        return NULL;
    }

    hFile->handle = NULL;
    hFile->header = NULL;
    hFile->fileIsOpen = false;
    hFile->samples = NULL;
    hFile->formatChunkFound = false;
    hFile->dataChunkFound = false;
    hFile->sampleCount = 0;

    hFile->handle = fopen(filename, "rb");

    if (!hFile->handle) {
        wavFormatClose(hFile);
        return NULL;
    }

    hFile->fileIsOpen = true;

    char buffer[4] = {0};
    size_t bytesRead = fread(&buffer, 1, 4, hFile->handle);
    if ( bytesRead != 4 || strcmp("RIFF", buffer) != 0 )  {
        wavFormatClose(hFile);
        return NULL;
    }

    fseek(hFile->handle, 4L, SEEK_CUR);

    bytesRead = fread(&buffer, 1, 4, hFile->handle);
    if (bytesRead != 4 || strcmp("WAVE", buffer) != 0) {
        wavFormatClose(hFile);
        return NULL;
    }

    return hFile;
}

void wavFormatReadAll(WavFile * hFile) {
    while (wavFormatProcessNextChunk(hFile)) {
        continue;
    }
}

bool wavFormatIsReady(WavFile * hFile) {
    return hFile->formatChunkFound && 
        hFile->dataChunkFound && 
        feof(hFile->handle) &&
        hFile->fileIsOpen;
}

size_t wavFormatGetSampleCount(WavFile * hFile) {
    // TODO: Check non null values for hFile and header
    if  ( hFile->sampleCount != 0 ) { 
        return hFile->sampleCount;
    }
    if ( hFile->dataLength > 0 && 
        hFile->header->bitsPerSample > 0 &&
        hFile->dataLength / hFile->header->bitsPerSample > 0 ) {
        hFile->sampleCount = hFile->dataLength / (hFile->header->bitsPerSample / 8);
        return hFile->sampleCount;
    }
    return 0;
}

float wavFormatGetSample(WavFile * hFile, size_t sample) {
    if ( !wavFormatIsReady(hFile) ) {
        return NAN;
    }
    if ( sample > wavFormatGetSampleCount(hFile) ) {
        return NAN;
    }
    return hFile->samples[sample];
}

bool wavFormatSetSample(WavFile * hFile, size_t sample, float value) { 
    if ( !wavFormatIsReady(hFile) ) {
        return false;
    }
    if ( sample > wavFormatGetSampleCount(hFile) ) {
        return false;
    }
    hFile->samples[sample] = value;
    return true;
}

uint16_t wavFormatGetBitsPerSample(WavFile * hFile) {
    if ( !wavFormatIsReady(hFile) ) {
        return 0;
    }
    if ( hFile->header->bitsPerSample > 0 ) {
        return hFile->header->bitsPerSample;
    }
    return 0;
}

/* This signature is a direct copy of fwrite()
This function writes and then returns a boolean on successfully writing. */
bool fwriteAndVerify(const void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __s) {
    return fwrite(__ptr, __size, __n, __s) == __n;
}
/* This closes the open file handle and removes the file */
void cleanupAndRemove(FILE * file, const char * filename) {
    fclose(file);
    remove(filename);
}

bool wavFormatSave(WavFile * hFile, const char * filename) {
    FILE * file = fopen(filename, "wb");

    uint32_t size = 8 + sizeof(WavHeader) + 8 + hFile->dataLength;
    size_t verify = 0;
    uint32_t PCM = 16;

    if (!fwriteAndVerify("RIFF", 1, 4, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }
    if (!fwriteAndVerify(&(size), 4, 1, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }
    if (!fwriteAndVerify("WAVE", 1, 4, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }
    if (!fwriteAndVerify("fmt ", 1, 4, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }
    if (!fwriteAndVerify(&PCM, 1, 4, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }
    if (!fwriteAndVerify(hFile->header, sizeof(WavHeader), 1, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }


    if (!fwriteAndVerify("data", 1, 4, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }
    if (!fwriteAndVerify(&(hFile->dataLength), 1, 4, file)) {
        cleanupAndRemove(file, filename);
        return false;
    }

    size_t i = 0;
    size_t expectedSize =  hFile->header->bitsPerSample / 8;
    size_t bytesWritten = expectedSize;
    size_t totalBytesWritten = 0;
    
    while ( totalBytesWritten < hFile->dataLength && bytesWritten == expectedSize){
        if ( expectedSize == 1 ) {
            uint8_t sample = (uint8_t)((hFile->samples[i++]+1.F) / U8_CONVERSION_FACTOR);
            bytesWritten = fwrite(&sample, 1, expectedSize, file);
        }
        else if ( expectedSize == 2 ) {
            int16_t sample = (int16_t)(hFile->samples[i++] / I16_CONVERSION_FACTOR);
            bytesWritten = fwrite(&sample, 1, expectedSize, file);
        }
        else if ( expectedSize == 4 ) { 
            int32_t sample = (int32_t)(hFile->samples[i++] / I16_CONVERSION_FACTOR);
            bytesWritten = fwrite(&sample, 1, expectedSize, file);
        }
        else { 
            // We can only handle 1,2, or 4 byte samples.
            cleanupAndRemove(file, filename);
            return false;
        }
        totalBytesWritten += bytesWritten;

    }
    if ( totalBytesWritten != hFile->dataLength || bytesWritten != expectedSize ) { 
        // Either a write failed or we didn't get the the entire sample set.
        cleanupAndRemove(file, filename);
        return false;
    }
    fclose(file);
    return true;
}