//
//  echoprint-codegen
//  Copyright 2011 The Echo Nest Corporation. All rights reserved.
//


#include <sstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include "Codegen.h"
#include "AudioBufferInput.h"
#include "Fingerprint.h"
#include "Whitening.h"
#include "SubbandAnalysis.h"
#include "Fingerprint.h"
#include "Common.h"

using std::string;
using std::vector;

Codegen::Codegen(const float* pcm, uint numSamples, int start_offset) {
    if (Params::AudioStreamInput::MaxSamples < (uint)numSamples)
        throw std::runtime_error("File was too big\n");

    Whitening *pWhitening = new Whitening(pcm, numSamples);
    pWhitening->Compute();

    AudioBufferInput *pAudio = new AudioBufferInput();
    pAudio->SetBuffer(pWhitening->getWhitenedSamples(), pWhitening->getNumSamples());

    SubbandAnalysis *pSubbandAnalysis = new SubbandAnalysis(pAudio);
    pSubbandAnalysis->Compute();

    Fingerprint *pFingerprint = new Fingerprint(pSubbandAnalysis, start_offset);
    pFingerprint->Compute();

    //_CodeString = createCodeString(pFingerprint->getCodes());
    _JsonCodes = createJsonCodes(pFingerprint->getCodes());
    _NumCodes = pFingerprint->getCodes().size();

    delete pFingerprint;
    delete pSubbandAnalysis;
    delete pWhitening;
    delete pAudio;
}

string Codegen::createJsonCodes(vector<FPCode> vCodes)
{
  // Ashwin - Added this function to return list of codes
  // as Json list (as a string)
  std::ostringstream codestream;
  codestream << "[[";
  for (uint i = 0; i < vCodes.size(); i++) {
    codestream << vCodes[i].frame;
    if (i != vCodes.size() - 1)
      codestream << ",";
  }
  codestream << "],\n[";
  
  for (uint i = 0; i < vCodes.size(); i++) {
    int hash = vCodes[i].code;
    codestream << hash;
    if (i != vCodes.size() - 1)
      codestream << ",";
  }
  codestream << "]]\n";

  return codestream.str();
  
}
