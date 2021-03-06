//
//  echoprint-codegen
//  Copyright 2011 The Echo Nest Corporation. All rights reserved.
//



#include <stddef.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#include <endian.h>
#define POPEN_MODE "r"
#else
#include "win_unistd.h"
#include <winsock.h>
#define POPEN_MODE "rb"
#endif
#include <string.h>

#include "AudioStreamInput.h"
#include "Common.h"
#include "Params.h"

using std::string;

#define DBG_INPUT 0

namespace FFMPEG {
    // Do we think FFmpeg will read this as an audio file?
    bool IsAudioFile(const char* pFileName) {
        static const char* supportedExtensions[] = {".mp3", ".m4a", ".mp4", ".aif", ".aiff", ".flac", ".au", ".wav", ".aac", ".flv"};
        // Not an exhaustive list. ogg and rm could be added if tested.
        for (uint i = 0; i < NELEM(supportedExtensions); i++) {
            if (File::ends_with(pFileName, supportedExtensions[i]))
                return true;
        }
        return false;
    }
}

bool AudioStreamInput::IsSupported(const char *path) {
    return true; // Take a crack at anything, by default. The worst thing that will happen is that we fail.
}

AudioStreamInput::AudioStreamInput() : _pSamples(NULL), _NumberSamples(0), _Offset_s(0), _Seconds(0) {}

AudioStreamInput::~AudioStreamInput() {
    if (_pSamples != NULL)
        delete [] _pSamples, _pSamples = NULL;
}


bool AudioStreamInput::ProcessFile(const char* filename, int offset_s/*=0*/, int seconds/*=0*/) {
    if (!File::Exists(filename) || !IsSupported(filename))
        return false;

    _Offset_s = offset_s;
    _Seconds = seconds;
		_Filename = filename;
    std::string message = GetCommandLine(filename);

    FILE* fp = popen(message.c_str(), POPEN_MODE);
    bool ok = (fp != NULL);
    if (ok)
    {
        bool did_work = ProcessFilePointer(fp);
        bool succeeded = !pclose(fp);
        ok = did_work && succeeded;
    }
    else
        fprintf(stderr, "AudioStreamInput::ProcessFile can't open %s\n", filename);

    return ok;
}

// reads raw signed 16-bit shorts from a file
bool AudioStreamInput::ProcessRawFile(const char* rawFilename) {
    FILE* fp = fopen(rawFilename, "r"); // TODO: Windows
    bool ok = (fp != NULL);
    if (ok)
    {
        ok = ProcessFilePointer(fp);
        fclose(fp);
    }

    return ok;
}

unsigned int MurmurHashNeutral2 ( const void * key, int len, unsigned int seed );
// reads raw signed 16-bit shorts from stdin, for example:
// ffmpeg -i fille.mp3 -f s16le -ac 1 -ar 11025 - | TestAudioSTreamInput

bool AudioStreamInput::ProcessStandardInput(uint numSamples, 
																						const std::string filename) {
    // TODO - Windows will explodey at not setting O_BINARY on stdin.
		_Filename = filename; // for debug purposes
		return ProcessFilePointer(stdin, numSamples);
}

bool AudioStreamInput::ProcessFilePointer(FILE* pFile, uint numSamples) {
    std::vector<short*> vChunks;
    uint nSamplesPerChunk = (uint) Params::AudioStreamInput::SamplingRate * Params::AudioStreamInput::SecondsPerChunk;
    uint samplesRead = 0;
    while (true) {
        short* pChunk = new short[nSamplesPerChunk];
        samplesRead = fread(pChunk, Params::AudioStreamInput::BytesPerSample2,
														nSamplesPerChunk, pFile);
//				uint bufHash = MurmurHashNeutral2(pChunk, samplesRead * Params::AudioStreamInput::BytesPerSample2, HASH_SEED);

//				fprintf(stderr, "***Read: %d, hash: %d\n", samplesRead, bufHash);
				if (samplesRead == 0) {
						delete[] pChunk;
						break;
				}
				_NumberSamples += samplesRead;
				vChunks.push_back(pChunk);
				if (numSamples != 0 && _NumberSamples >= numSamples) break;
    }

    // Convert from shorts to 16-bit floats and copy into sample buffer.
    uint sampleCounter = 0;
    _pSamples = new float[_NumberSamples];
    uint samplesLeft = _NumberSamples;

#if DBG_INPUT==1
		// open debug output file
		std::string fname = _Filename + ".s16le";
		FILE* _debug_fout = fopen(fname.c_str(), "w");
#endif

    for (uint i = 0; i < vChunks.size(); i++)
    {
        short* pChunk = vChunks[i];
        uint numSamples = samplesLeft < nSamplesPerChunk ? samplesLeft : nSamplesPerChunk;
				//fprintf(stderr, "numSamples: %d\n", numSamples);

        for (uint j = 0; j < numSamples; j++) {
            _pSamples[sampleCounter++] = (float) le16toh(pChunk[j]) / 32768.0f;

#if DBG_INPUT==1
						fwrite(pChunk + j, 2, 1, _debug_fout);
#endif
				}

        samplesLeft -= numSamples;
        delete [] pChunk, vChunks[i] = NULL;


    }
    assert(samplesLeft == 0);

#if DBG_INPUT==1
		fclose(_debug_fout);
#endif

    int error = ferror(pFile);
    bool success = error == 0;

    if (!success)
        perror("ProcessFilePointer error");
    return success;
}


