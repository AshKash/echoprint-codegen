//
//  echoprint-codegen
//  Copyright 2011 The Echo Nest Corporation. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <memory>
#ifndef _WIN32
#include <libgen.h>
#include <dirent.h>
#endif
#include <stdlib.h>
#include <stdexcept>

#include "AudioStreamInput.h"
#include "Metadata.h"
#include "Codegen.h"
#include <string>
#define MAX_FILES 200000

using namespace std;

// deal with quotes etc in json
std::string escape(const string & value)
{
    std::string s(value);
    std::string out = "";
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
	char c = s[i];
	if (c <= 31)
	    continue;

	switch (c) {
	case '"':
	    out += "\\\"";
	    break;
	case '\\':
	    out += "\\\\";
	    break;
	case '\b':
	    out += "\\b";
	    break;
	case '\f':
	    out += "\\f";
	    break;
	case '\n':
	    out += "\\n";
	    break;
	case '\r':
	    out += "\\r";
	    break;
	case '\t':
	    out += "\\t";
	    break;
	    // case '/' : out += "\\/" ; break; // Unnecessary?
	default:
	    out += c;
	    // TODO: do something with unicode?
	}
    }

    return out;
}

char *json_string_for_file(char *filename, char *filename2, int start_offset, int duration)
{
    // Given a filename, do all the work to get a JSON string output.
    double t1 = now();
    AudioStreamInput * pAudio;
	uint sampleRate;
	uint bitrate;
	uint numSamples;
	uint duration2;

	if (strcmp(filename, "-") == 0) { 
		// Process stdin
		sampleRate = 11025;   // samples per second
		bitrate = sampleRate * 16 / 1024;     // kb/s
		numSamples = sampleRate * duration;
		pAudio = new StdinStreamInput();
		pAudio->ProcessStandardInput(numSamples);

		// Need to read limited number of samples and write out json
		// so as to make it work in streaming fashion
	} else {
		pAudio = new FfmpegStreamInput();
		pAudio->ProcessFile(filename, start_offset, duration);
		// Get the ID3 tag information.
		auto_ptr < Metadata > pMetadata(new Metadata(filename));
		bitrate = pMetadata->Bitrate();
		sampleRate = pMetadata->SampleRate();
	    duration2 = pMetadata->Seconds();
	}
	numSamples = pAudio->getNumSamples();
	duration2 = numSamples / sampleRate;

    if (pAudio == NULL) {	// Unable to decode!
		throw std::runtime_error("Could not create decoder");
    }

    if (numSamples < 1) {
		delete pAudio;
		throw std::runtime_error("Could not create decode file (eof?)");
    }
    t1 = now() - t1;

    double t2 = now();
    auto_ptr < Codegen >
		pCodegen(new Codegen(pAudio->getSamples(), numSamples, start_offset));
    t2 = now() - t2;


    // preamble + codelen
    char *output =
	(char *) malloc(sizeof(char) * (16384 + strlen(pCodegen->getJsonCodes().c_str())));

    sprintf(output, "{\"bitrate\":%d, "
			"\"sample_rate\":%d, "
			"\"duration\":%d, "
			"\"filename\":\"%s\",\n"
			"\"samples_decoded\":%d, "
			"\"given_duration\":%d, "
			"\"start_offset\":%d, "
			"\"version\":%2.2f,\n"
			"\"codegen_time\":%2.6f, "
			"\"decode_time\":%2.6f, "
			"\"code_count\":%d,\n"
			"\"code\":%s}",
			bitrate,
			sampleRate,
			duration2,
			escape(filename2).c_str(),
			numSamples,
			duration,
			start_offset,
			pCodegen->getVersion(),
			t2,
			t1, pCodegen->getNumCodes(), pCodegen->getJsonCodes().c_str()
		);

	delete pAudio;
    return output;
}

void usage()
{
	fprintf(stderr,
			"Usage: codegen [ filename [seconds_start] [seconds_duration] | "
			" - -f filename [-i <interval seconds>]\n");
	fprintf(stderr, "if - -f is used, a new file will be written out every interval seconds\n");
	exit(-1);

}

int main(int argc, char **argv)
{
    if (argc < 2) usage();

    try {
		char *filename = argv[1];
		char *filename2 = NULL;  // What we put in the JSON output
		int start_offset = 0;
		int duration = 0;
		int interval = 0;
		if (strcmp(filename, "-") == 0) {
			if (argc < 4) usage();
			if (strcmp(argv[2], "-f") != 0) usage();
			filename2 = argv[3];
			if (argc > 4) {
				if (strcmp(argv[4], "-i") != 0) {
					usage();
				}
				if (argc > 5) {
					interval = atoi(argv[5]);
				} else { 
					usage();
				}
			}

		} else {
			filename2 = filename;
			if (argc > 2)
				start_offset = atoi(argv[2]);
			if (argc > 3)
				duration = atoi(argv[3]);
		}
		
		if (interval != 0) {
			// this automatically means that we are reading from stdin
			fprintf(stderr, "Writing output to %s-now().json\n", filename2);
			while (true) {
				long long t1 = now2();
				char *output = json_string_for_file(filename, filename2, 
													start_offset, interval);
				// write this out to filename2
				char fname[256];
				sprintf(fname, "%s-%lld.json", filename2, t1);
				FILE * fOutput = fopen(fname,  "w");
				if (!fOutput) throw std::runtime_error("Cannot open file for writing");
				fwrite(output, strlen(output), 1, fOutput);
				//printf("%s", output);
				fclose(fOutput);
			}
		} else {
			char *output = json_string_for_file(filename, filename2, 
												start_offset, duration);
			printf("%s", output);
		}
	}
	
    catch(std::runtime_error & ex) {
		fprintf(stderr, "%s\n", ex.what());
		return 1;
    }
    catch( ...) {
		fprintf(stderr, "Unknown failure occurred\n");
		return 2;
    }
}
