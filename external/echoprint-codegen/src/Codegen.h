//
//  echoprint-codegen
//  Copyright 2011 The Echo Nest Corporation. All rights reserved.
//


#ifndef CODEGEN_H
#define CODEGEN_H

// Entry point for generating codes from PCM data.
// This is specific to Tvgram - Ashwin :)
#define VERSION 5.00

#include <string>
#include <vector>
#include <sys/types.h>

#ifdef _MSC_VER
    #ifdef CODEGEN_EXPORTS
        #define CODEGEN_API __declspec(dllexport)
        #pragma message("Exporting codegen.dll")
    #else
        #define CODEGEN_API __declspec(dllimport)
        #pragma message("Importing codegen.dll")
    #endif
#else
    #define CODEGEN_API
#endif

class Fingerprint;
class SubbandAnalysis;
struct FPCode;

class CODEGEN_API Codegen {
public:
    Codegen(const float* pcm, uint numSamples, int start_offset);

    //string getCodeString(){return _CodeString;}
	std::string getJsonCodes(){return _JsonCodes;}
    int getNumCodes(){return _NumCodes;}
    float getVersion() { return VERSION; }
private:
    Fingerprint* computeFingerprint(SubbandAnalysis *pSubbandAnalysis, int start_offset);
    //string createCodeString(vector<FPCode> vCodes);
	std::string createJsonCodes(std::vector<FPCode> vCodes);

	std::string compress(const std::string& s);
    //string _CodeString;
	std::string _JsonCodes;
    int _NumCodes;
};

#endif
