#include "sio/language_model.h"

#include "gtest/gtest.h"

//#include "sio/dbg.h"

namespace sio {

TEST(LanguageModel, PrefixLm) {
    Tokenizer tokenizer;
    tokenizer.Load("testdata/tokenizer.vocab");
    //dbg(tokenizer.index_to_token);

    PrefixLm prefix_lm;
    prefix_lm.Load();

    LanguageModel* m = &prefix_lm;

    LmStateId null_state = m->NullState();
    EXPECT_EQ(null_state, 0); // null state is index as 0

    LmScore s;

    LmStateId bos_state;
    m->GetScore(null_state, tokenizer.Index("<s>"), &s, &bos_state);
    EXPECT_EQ(bos_state, 1);

    LmStateId a;
    m->GetScore(bos_state, tokenizer.Index("a"), &s, &a);
    EXPECT_EQ(a, 2);

    LmStateId aa;
    m->GetScore(a, tokenizer.Index("a"), &s, &aa);
    EXPECT_EQ(aa, 3);

    LmStateId ab;
    m->GetScore(a, tokenizer.Index("b"), &s, &ab);
    EXPECT_EQ(ab, 4);

    LmStateId ab_eos;
    m->GetScore(ab, tokenizer.Index("</s>"), &s, &ab_eos);
    EXPECT_EQ(ab_eos, 5);

}

} // namespace sio
