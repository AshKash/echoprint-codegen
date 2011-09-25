//
//  echoprint-codegen
//  Copyright 2011 The Echo Nest Corporation. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <memory>
#include <string>
#ifndef _WIN32
#include <libgen.h>
#include <dirent.h>
#endif
#include <stdlib.h>
#include <stdexcept>

#include "AudioStreamInput.h"
#include "Metadata.h"
#include "Codegen.h"
#include "Params.h"

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
    auto_ptr <AudioStreamInput> pAudio;
		uint sampleRate;
		uint numSamples;
		uint duration2;
		
		if (strcmp(filename, "-") == 0) { 
				// Process stdin
				sampleRate = (uint) Params::AudioStreamInput::SamplingRate;
				numSamples = sampleRate * duration;
				pAudio.reset( new StdinStreamInput() );
				pAudio->ProcessStandardInput(numSamples);
				
				// Need to read limited number of samples and write out json
				// so as to make it work in streaming fashion
		} else {
				pAudio.reset( new FfmpegStreamInput() );
				pAudio->ProcessFile(filename, start_offset, duration);
				sampleRate = (uint) Params::AudioStreamInput::SamplingRate;
		}
		numSamples = pAudio->getNumSamples();
		duration2 = numSamples / sampleRate;
		
    if (pAudio.get() == NULL) {	// Unable to decode!
				throw std::runtime_error("Could not create decoder");
    }
		
    if (numSamples < 1) {
				//throw std::runtime_error("Could not create decode file (eof?)");
				// Silently ignore this as it is most likely the last few bytes before eof...
				return NULL;
    }
    t1 = now() - t1;
		
    double t2 = now();
    auto_ptr < Codegen >
				pCodegen(new Codegen(pAudio->getSamples(), numSamples, start_offset));
    t2 = now() - t2;

		
    // preamble + codelen
    char *output =
				(char *) malloc(sizeof(char) * (16384 + strlen(pCodegen->getJsonCodes().c_str())));
		
    sprintf(output, "{\"duration\":%d, "
						"\"filename\":\"%s\",\n"
						"\"samples_decoded\":%d, "
						"\"given_duration\":%d, "
						"\"start_offset\":%d, "
						"\"version\":%2.2f,\n"
						"\"codegen_time\":%2.6f, "
						"\"decode_time\":%2.6f, "
						"\"code_count\":%d,\n"
						"\"code\":%s}",
						duration2,
						escape(filename2).c_str(),
						numSamples,
						duration,
						start_offset,
						pCodegen->getVersion(),
						t2,
						t1, pCodegen->getNumCodes(), pCodegen->getJsonCodes().c_str()
				);
		
    return output;
}

void usage()
{
		fprintf(stderr,
						"Usage: codegen [ filename [seconds_start] [seconds_duration]] | "
						" - -f filename [-i <interval seconds>] [-j <json filename>]\n");
		fprintf(stderr, 
						"if - -f is used, a new file will be written out every interval "
						"seconds\n");
		exit(-1);
}

int main(int argc, char **argv)
{
    if (argc < 2) usage();
		
    try {
				char *in_fname = argv[1];
				char *out_fname = NULL;  // Save file (when - -f given)
				char *json_fname = NULL; // JSON file (when - -f given)
				int start_offset = 0;
				int duration = 0;
				int interval = 0;
				if (strcmp(in_fname, "-") == 0) {
						if (argc < 4) usage();
						if (strcmp(argv[2], "-f") != 0) usage();
						out_fname = argv[3];
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
						if (argc > 6) {
								if (strcmp(argv[6], "-j") != 0) {
										usage();
								}
								if (argc > 7) {
										json_fname = argv[7];
								} else { 
										usage();
								}
						}
						
						if (!json_fname) json_fname = out_fname;
						
				} else {
						out_fname = in_fname;
						if (argc > 2)
								start_offset = atoi(argv[2]);
						if (argc > 3)
								duration = atoi(argv[3]);
				}
				
				if (interval != 0) {
						// this automatically means that we are reading from stdin
						int i = 0;
						while (true) {
								char *output = json_string_for_file(in_fname, json_fname, 
																										start_offset, interval);
								if (!output) break;
								
								// write this out to out_fname
								char fname[256];
								int out_len = strlen(output);
								sprintf(fname, "%s#%d.json", out_fname, i++);
								//fprintf(stderr, "Writing output to %s, %d\n", fname, out_len);
								FILE * fOutput = fopen(fname,  "w");
								if (!fOutput) {
										fprintf(stderr, "Cannot open file for writing");
										free(output);
										continue;
								}
								
								fwrite(output, out_len, 1, fOutput);
								//printf("%s", output);
								fclose(fOutput);
								free(output);
						}
				} else {
						char *output = json_string_for_file(in_fname, out_fname, 
																								start_offset, duration);
						printf("%s", output);
						free(output);
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
		
		return 0;
}
