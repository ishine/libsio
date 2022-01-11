#ifndef SIO_FEATURE_H
#define SIO_FEATURE_H

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
    FeatureExtractorConfig config;

    // need pointer here because we want Reset() functionality
    Owner<kaldi::OnlineBaseFeature*> extractor = nullptr;

    // mvn object is stateless, owned outside, not here
    Optional<const MeanVarNorm*> mean_var_norm = nullptr;

    //[0, cur_frame) popped frames, [cur_frame, NumFramesReady()) remainder frames.
    index_t cur_frame = 0;


    ~FeatureExtractor() noexcept {
        Delete(extractor);
    }


    Error Load(const FeatureExtractorConfig& c, const MeanVarNorm* mvn = nullptr) { 
        SIO_CHECK_EQ(c.type, "fbank");

        config = c;

        if (extractor != nullptr) {
            SIO_WARNING << "Feature extractor existed, releasing old one.";
            Delete(extractor);
        }
        extractor = new kaldi::OnlineFbank(config.fbank);

        mean_var_norm = mvn;

        cur_frame = 0;

        return Error::OK;
    }


    Error Reset() {
        Delete(extractor);
        extractor = new kaldi::OnlineFbank(config.fbank);
        cur_frame = 0;

        return Error::OK;
    }


    void Push(const float* samples, size_t num_samples, float sample_rate) {
        extractor->AcceptWaveform(
            sample_rate, 
            kaldi::SubVector<float>(samples, num_samples)
        );
    }


    void PushEnd() {
        extractor->InputFinished();
    }


    Vec<float> Pop() {
        SIO_CHECK_GT(Len(), 0);
        Vec<float> feat_frame(Dim(), 0.0f);

        // kaldi_frame is a helper frame view, no underlying data ownership
        kaldi::SubVector<float> kaldi_frame(feat_frame.data(), feat_frame.size());
        extractor->GetFrame(cur_frame++, &kaldi_frame);
        if (mean_var_norm) {
            mean_var_norm->Normalize(&kaldi_frame);
        }

        return std::move(feat_frame);
    }


    size_t Dim() const {
        return extractor->Dim();
    }


    size_t Len() const {
        return extractor->NumFramesReady() - cur_frame;
    }


    float SampleRate() const {
        return config.fbank.frame_opts.samp_freq;
    }


    float FrameRate() const {
        return 1000.0f / config.fbank.frame_opts.frame_shift_ms;
    }

}; // struct FeatureExtractor
}  // namespace sio
#endif
