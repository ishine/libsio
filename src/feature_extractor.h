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

class FeatureExtractor {
private:
    FeatureExtractorConfig config_;

    // need pointer here because we want Reset() functionality
    Owner<kaldi::OnlineBaseFeature*> fbank_extractor_ = nullptr;

    // mvn object is stateless, so no need to claim ownership here, owned by SpeechToText
    Optional<const MeanVarNorm*> mean_var_norm_ = nullptr;

    //[0, cur_frame_) popped frames, [cur_frame_, NumFramesReady()) remainder frames.
    index_t cur_frame_ = 0;

public:

    ~FeatureExtractor() noexcept {
        delete fbank_extractor_; fbank_extractor_ = nullptr;
    }


    Error Setup(const FeatureExtractorConfig& config, const MeanVarNorm* mean_var_norm = nullptr) { 
        SIO_CHECK_EQ(config.type, "fbank");

        config_ = config;

        if (fbank_extractor_ != nullptr) {
            SIO_WARNING << "Try to setup already defined FeatureExtractor, have to release existing resource.";
            delete fbank_extractor_; fbank_extractor_ = nullptr;
        }
        fbank_extractor_ = new kaldi::OnlineFbank(config_.fbank);

        mean_var_norm_ = mean_var_norm;

        cur_frame_ = 0;

        return Error::OK;
    }


    Error Reset() {
        delete fbank_extractor_;
        fbank_extractor_ = new kaldi::OnlineFbank(config_.fbank);
        cur_frame_ = 0;

        return Error::OK;
    }


    void Push(const float* samples, size_t num_samples, float sample_rate) {
        fbank_extractor_->AcceptWaveform(
            sample_rate, 
            kaldi::SubVector<float>(samples, num_samples)
        );
    }


    void PushEnd() {
        fbank_extractor_->InputFinished();
    }


    Vec<float> Pop() {
        SIO_CHECK_GT(Len(), 0);
        Vec<float> feat_frame(Dim(), 0.0f);

        // kaldi_frame is a helper frame view, no underlying data ownership
        kaldi::SubVector<float> kaldi_frame(feat_frame.data(), feat_frame.size());
        fbank_extractor_->GetFrame(cur_frame_++, &kaldi_frame);
        if (mean_var_norm_) {
            mean_var_norm_->Normalize(&kaldi_frame);
        }

        return std::move(feat_frame);
    }


    size_t Dim() const {
        return fbank_extractor_->Dim();
    }


    size_t Len() const {
        return fbank_extractor_->NumFramesReady() - cur_frame_;
    }


    float SampleRate() const {
        return config_.fbank.frame_opts.samp_freq;
    }


    float FrameRate() const {
        return 1000.0f / config_.fbank.frame_opts.frame_shift_ms;
    }

}; // class FeatureExtractor
}  // namespace sio
#endif
