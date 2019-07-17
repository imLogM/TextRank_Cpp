#include <fstream>
#include <iostream>

#include "src/text_ranker.h"

int main()
{
    std::ifstream textFile;
    textFile.open("example/01.txt");
    std::string text((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());

    TextRanker textRanker;
    vector<string> keySentences;
    const int topK = 3;
    textRanker.ExtractKeySentences(text, keySentences, topK);
    
    // write top 3 sentences from results
    for (int i=0; i<(int)keySentences.size(); ++i) {
        std::cout << keySentence << "\n";
    }


    textFile.close();
}
