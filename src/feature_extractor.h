#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

#include <memory>

#include "feat/online-feature.h"

#include "sio/common.h"
#include "sio/struct_loader.h"
#include "sio/mean_var_norm.h"

namespace sio {
struct FeatureExtractorConfig {
    std::string type; // support "fbank" only for now
    kaldi::FbankOptions fbank;

    Error Register(StructLoader* loader, const std::string module = "") {
        loader->AddEntry(module + ".type",         &type);
        loader->AddEntry(module + ".sample_rate",  &fbank.frame_opts.samp_freq);
        loader->AddEntry(module + ".dither",       &fbank.frame_opts.dither);
        loader->AddEntry(module + ".num_mel_bins", &fbank.mel_opts.num_bins);

        return Error::OK;
    }
};


struct FeatureExtractor {
    const FeatureExtractorConfig* config = nullptr;

    // need pointer here to support multiple types of extractors (e.g. fbank, mfcc)
    Unique<kaldi::OnlineBaseFeature *> extractor;

    Optional<const MeanVarNorm *> mean_var_norm = nullptr;

    //[0, cur_frame) popped frames, [cur_frame, NumFramesReady()) remainder frames.
    index_t cur_frame = 0;


    Error Load(const FeatureExtractorConfig& cfg, Optional<const MeanVarNorm*> mvn = nullptr) { 
        SIO_CHECK_EQ(cfg.type, "fbank");
        config = &cfg;

        SIO_CHECK(!extractor) << "Feature extractor initialized already.";
        extractor = make_unique<kaldi::OnlineFbank>(config->fbank);

        mean_var_norm = mvn;

        cur_frame = 0;

        return Error::OK;
    }


    void Push(const f32* samples, size_t num_samples, f32 sample_rate) {
        extractor->AcceptWaveform(
            sample_rate, 
            kaldi::SubVector<f32>(samples, num_samples)
        );
    }


    void PushEnd() {
        extractor->InputFinished();
    }


    Vec<f32> Pop() {
        SIO_CHECK_GT(Len(), 0);
        Vec<f32> feat_frame(Dim(), 0.0f);

        // kaldi_frame is a helper frame view, no underlying data ownership
        kaldi::SubVector<f32> kaldi_frame(feat_frame.data(), feat_frame.size());
        extractor->GetFrame(cur_frame++, &kaldi_frame);
        if (mean_var_norm) {
            mean_var_norm->Normalize(&kaldi_frame);
        }

        return std::move(feat_frame);
    }


    Error Reset() {
        SIO_CHECK_EQ(config->type, "fbank");
        extractor.reset();
        extractor = make_unique<kaldi::OnlineFbank>(config->fbank);
        cur_frame = 0;

        return Error::OK;
    }


    size_t Dim() const {
        return extractor->Dim();
    }


    size_t Len() const {
        return extractor->NumFramesReady() - cur_frame;
    }


    f32 SampleRate() const {
        SIO_CHECK(config != nullptr);
        return config->fbank.frame_opts.samp_freq;
    }


    f32 FrameRate() const {
        SIO_CHECK(config != nullptr);
        return 1000.0f / config->fbank.frame_opts.frame_shift_ms;
    }

}; // struct FeatureExtractor
}  // namespace sio
#endif
