#include "gtest/gtest.h"
#include "seq.h"
#include "minimizer.h"
#include "interval.h"
#include <stdint.h>
#include <iostream>

using namespace std;

class SeqTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
    // Code here will be called immediately after each test
    // (right before the destructor).
  }
};

TEST_F(SeqTest,sketchShortReads){
    Seq s1(0,"0", "AGCTAATGCGTT", 11, 3);
    Seq s2(0,"0", "AGCTAATGCGTT", 10, 3);
    Seq s3(0,"0", "AGCTAATGCGTT", 9, 3);
    Seq s4(0,"0", "AGCTAATGCATA", 9, 3);
    uint32_t j = 0;
    EXPECT_EQ(s1.sketch.size(),j) << "Have " << s1.sketch.size() << " minimizer when string is too short";
    ++j;
    EXPECT_EQ(s2.sketch.size(),j) << "Have " << s2.sketch.size() << " minimizers when should have 1";
    ++j;
    EXPECT_EQ(s3.sketch.size(),j) << "Have " << s3.sketch.size() << " minimizers when should have 2";
    j=1;
    EXPECT_EQ(s4.sketch.size(),j) << "Have " << s4.sketch.size() << " minimizers when should have 1";
}

TEST_F(SeqTest,sketchIncludesEveryLetter){
    Seq s1(0,"0", "AGCTAATGTGTT", 3, 3);
    Seq s2(0,"0", "AGCTAATGTGTT", 2, 3);
    Seq s3(0,"0", "AGCTAATGTGTT", 1, 3);
    Seq s4(0,"0", "AGCTAATGTGAT", 3, 3);

    set<int> pos_inc;
    for(set<Minimizer*>::iterator it=s4.sketch.begin(); it != s4.sketch.end(); ++it)
    {
        for (uint32_t j = (*it)->pos.start; j<(*it)->pos.end; ++j)
        {
            pos_inc.insert(j);
        }
        
    } 
    for (uint32_t i = 3-1; i!=12-3+1; ++i) // first or last w-1 may not be included
    {
	cout << "looking for pos " << i << endl;
	EXPECT_EQ((pos_inc.find(i)!=pos_inc.end()), true);
    }
    //set<int> expected = {0,1,2,3,4,5,6,7,8,9,10,11};
    //EXPECT_EQ(pos_inc, expected) << "sketch misses a letter";
 
    uint32_t j = 10;
    EXPECT_EQ(s3.sketch.size(), j) << "sketch with w=1 has incorrect size " << s3.sketch.size();
    
    pos_inc.clear();
    for(set<Minimizer*>::iterator it=s2.sketch.begin(); it != s2.sketch.end(); ++it)
    {
        for (uint32_t j = (*it)->pos.start; j<(*it)->pos.end; ++j)
        {
            pos_inc.insert(j);
        }
    }
    for (uint32_t i = 2-1; i!=12-2+1; ++i) // first or last w-1 may not be included
    {
        EXPECT_EQ((pos_inc.find(i)!=pos_inc.end()), true);
    }
    //EXPECT_EQ(pos_inc, expected) << "sketch for s2 includes/misses wrong letter";

    pos_inc.clear();
    for(set<Minimizer*>::iterator it=s1.sketch.begin(); it != s1.sketch.end(); ++it)
    {
        for (uint32_t j = (*it)->pos.start; j<(*it)->pos.end; ++j)
        {
            pos_inc.insert(j);         }
    }
    for (uint32_t i = 3-1; i!=12-3+1; ++i) // first or last w-1 may not be included
    {
        EXPECT_EQ((pos_inc.find(i)!=pos_inc.end()), true);
    }
    //expected = {0,1,2,3,4,5,6,7,8,9};
    //EXPECT_EQ(pos_inc, expected) << "sketch for s1 includes/misses wrong letter";

}

//TEST_F(SeqTest,sketchCorrect){
//}
