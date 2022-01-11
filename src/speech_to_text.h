#ifndef SIO_SPEECH_TO_TEXT_H
#define SIO_SPEECH_TO_TEXT_H

#include <torch/script.h>

#include "sio/common.h"
#include "sio/speech_to_text_config.h"
#include "sio/recognizer.h"
#include "sio/tokenizer.h"

namespace sio {
struct SpeechToText {
    SpeechToTextConfig config;
    torch::jit::script::Module nnet;
    Tokenizer tokenizer;
    std::unique_ptr<MeanVarNorm> mean_var_norm;


    Error Load(std::string config_file) { 
        config.Load(config_file);

        if (config.mean_var_norm != "") {
            SIO_CHECK(!mean_var_norm) << "mean_var_norm initialized already.";
            mean_var_norm = make_unique<MeanVarNorm>();
            mean_var_norm->Load(config.mean_var_norm);
        } else {
            mean_var_norm.reset();
        }

        tokenizer.Load(config.tokenizer_vocab);

        SIO_CHECK(config.nnet != "") << "stt nnet is required";
        SIO_INFO << "Loading torchscript nnet from: " << config.nnet; 
        nnet = torch::jit::load(config.nnet);

        return Error::OK;
    }


    Optional<Recognizer*> CreateRecognizer() {
        try {
            Recognizer* rec = new Recognizer;
            rec->Load(
                config.feature_extractor, mean_var_norm.get(), /* feature */
                tokenizer, /* tokenizer */ 
                config.scorer, nnet /* scorer */
            ); 
            return rec;
        } catch (...) {
            return nullptr;
        }
    }


    void DestroyRecognizer(Recognizer* rec) {
        SIO_CHECK(rec != nullptr);
        delete rec;
    }

}; // class SpeechToText
}  // namespace sio

#endif
