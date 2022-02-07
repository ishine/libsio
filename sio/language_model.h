#ifndef SIO_LANGUAGE_MODEL_H
#define SIO_LANGUAGE_MODEL_H

#include "sio/common.h"
#include "sio/tokenizer.h"
//#include "sio/dbg.h"

namespace sio {

using LmStateId = i32;
using LmWordId = i32;
using LmScore = f32;


class LanguageModel {
public:
    virtual LmWordId Bos() const = 0;
    virtual LmWordId Eos() const = 0;
    virtual LmWordId Unk() const = 0;

    virtual LmStateId NullState() const = 0;

    virtual bool GetScore(LmStateId src, LmWordId word, LmScore* score, LmStateId* dst) = 0;

    virtual ~LanguageModel() { }
};


class PrefixLm : public LanguageModel {
    // prefix word sequence is represented with a unique hash of type size_t
    using PrefixHash = size_t;

    const Tokenizer* tokenizer_ = nullptr;

    Vec<PrefixHash> state_to_hash_;
    Map<PrefixHash, LmStateId> hash_to_state_;

public:

    Error Load(const Tokenizer& tokenizer) {
        SIO_CHECK(tokenizer_ == nullptr);
        tokenizer_ = &tokenizer;

        // Establish null lm state: state index = 0, prefix hash = 0
        SIO_CHECK(state_to_hash_.empty() && hash_to_state_.empty());
        state_to_hash_.push_back(0);
        hash_to_state_[0] = 0;

        //dbg(hash_to_state_);
        //dbg(state_to_hash_);

        return Error::OK;
    }


    LmWordId Bos() const override { return tokenizer_->bos; }
    LmWordId Eos() const override { return tokenizer_->eos; }
    LmWordId Unk() const override { return tokenizer_->unk; }


    LmStateId NullState() const override {
        SIO_CHECK(!hash_to_state_.empty()) << "PrefixLm uninitialzed ?";
        return 0;
    }


    bool GetScore(LmStateId src_state, LmWordId word, LmScore* score, LmStateId* dst_state) override {
        // prime are picked from Kaldi's VectorHasher:
        //   https://github.com/kaldi-asr/kaldi/blob/master/src/util/stl-utils.h#L230
        const int prime = 7853;

        *score = 0.0;

        size_t src_hash = state_to_hash_[src_state];
        size_t dst_hash = src_hash * prime + word; // incremental sequence hashing

        auto it = hash_to_state_.find(dst_hash);
        if (it == hash_to_state_.end()) {
            LmStateId s = state_to_hash_.size();
            state_to_hash_.push_back(dst_hash);
            hash_to_state_[dst_hash] = s;

            *dst_state = s;
        } else {
            *dst_state = it->second;
        }

        return true;
    }

}; // class PrefixLm

} // namespace sio
#endif
