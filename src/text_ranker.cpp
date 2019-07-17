#include "src/text_ranker.h"
#include <string>
#include <vector>
#include <tr1/unordered_set>
#include <cmath>
#include <set>

using namespace std;

static bool PairComp(std::pair<int, double> a, std::pair<int, double> b) 
{
    return a.second > b.second;
}

bool TextRanker::ExtractKeySentences(const std::string& input, std::vector<string>& outputs, int topK) 
{
    outputs.clear();
    if (input.empty() || topK < 1) {
        return false;
    }

    // TextRank
    bool ret = true;
    ret &= ExtractSentences(input, mSentences);
    ret &= BuildGraph(mSentences);
    ret &= CalcSentenceScores();

    if (!ret) {
        return false;
    }

    // 返回得分高的句子
    int kDim = mSentences.size();
    std::vector< std::pair<int, double> > visitPairs;  // (id, score)
    for(int i=0; i<kDim; ++i) {
        visitPairs.push_back(std::pair<int, double>(i, mScores[i]));
    }

    std::sort(visitPairs.begin(), visitPairs.end(), PairComp);

    for(int i=0; i<topK && i<kDim; ++i) {
        int id = visitPairs[i].first;
        outputs.push_back(mSentences[id]);
    }
    return true;
}

bool TextRanker::ExtractSentences(const std::string& input, std::vector<std::string>& outputs) 
{
    outputs.clear();
    if (input.empty()) { 
        outputs.push_back("");
        return false; 
    }

    static const int maxTextLen = 10000;  // 最大字符数（需考虑分词符、UTF编码等）
    std::string tempInput;
    if ((int)input.size() > maxTextLen) {
        tempInput = input.substr(0, maxTextLen);  // 文章过长则截断
    } else {
        tempInput = input;
    }
    xxxx::toLowerCase(tempInput);

    // 将所有的句尾标点替换为`.`
    static const std::string punctuations[] = {"?", "!", ".", ";", "？", "！", "。", "；", "……", "…", "\n"};
    for (int i=0; i<(int)(sizeof(punctuations)/sizeof(punctuations[0])); ++i){
        std::string punc = punctuations[i];
        StringReplaceAll(tempInput, punc, ".");
    }

    // 切分句子
    static const int minSentenceLen = 30;   // 单句话的最小字符数（需考虑分词符、UTF编码等）
    vector<std::string> tempOutput = xxxx::split(tempInput, ".");
    vector<std::string> tempOutput2;
    for (int i=0; i<(int)tempOutput.size(); ++i) {
        if ((int)tempOutput[i].size() > minSentenceLen) {
            tempOutput2.push_back(tempOutput[i]);   // 单句话的字符数过少，舍弃
        }
    }

    // 去重
    RemoveDuplicates(tempOutput2, outputs);

    // 句子过多则截断
    static const int maxSentencesNum = 50;
    if ((int)outputs.size() > maxSentencesNum) {
        outputs.resize(maxSentencesNum);
    }

    return true;
}

bool TextRanker::RemoveDuplicates(const std::vector<std::string>& inputs, std::vector<std::string>& outputs)
{
    outputs.clear();

    std::tr1::unordered_set<std::string> s(inputs.begin(), inputs.end());
    outputs = std::vector<std::string>(s.begin(), s.end());  

    return true;
}

bool TextRanker::BuildGraph(const std::vector<std::string>& sentences)
{
    if (sentences.empty()) { return false; }

    int kDim = sentences.size();

    // 计算邻接矩阵
    mAdjacencyMatrix.clear();
    mAdjacencyMatrix.resize(kDim, std::vector<double>(kDim, 0.0));

    InitWordSet(sentences); // 每个句子所包含的词提前做成`set`，加速 GetSimilarity 的计算。

    for(int i = 0; i < kDim - 1; i++)
    {
        for(int j = i + 1; j < kDim; j++)
        {
            double similarity = GetSimilarity(i, j);
            // the similarity matrix is symmetrical, so transposes are filled in with the same similarity
            mAdjacencyMatrix[i][j] = similarity;
            mAdjacencyMatrix[j][i] = similarity;
        }
    }

    // 统计每个节点出链的权重和
    mOutWeightSum.clear();
    mOutWeightSum.resize(kDim, 0.0);

    for (int i=0; i<kDim; ++i) {
        for (int j=0; j<kDim; ++j) {
            if (i==j) { continue; }
            mOutWeightSum[i] += mAdjacencyMatrix[i][j];
        }
    }

    return true;
}

bool TextRanker::InitWordSet(const std::vector<std::string>& sentences)
{
    int kDim = sentences.size();
    if (sentences.empty()) {
        return false;
    }

    mWordSets.clear();
    mWordSets.resize(kDim, std::set<std::string>());
    mWordSizes.clear();
    mWordSizes.resize(kDim, 0);

    for (int i=0; i<kDim; ++i) {
        std::vector<std::string> aWords = xxxx::split(sentences[i], "\t"); 
        mWordSizes[i] = aWords.size();
        std::set<std::string> aWordSet(aWords.begin(), aWords.end());
        mWordSets[i] = aWordSet;
    }

    return true;
}

double TextRanker::GetSimilarity(int a, int b)
{
    if ((int)mWordSets.size() <= a || (int)mWordSets.size() <= b || mWordSets.size() != mWordSizes.size()) {
        return 0.0;
    }

    std::vector<std::string> commonWords;
    std::set_intersection
    (
        mWordSets[a].begin(),
        mWordSets[a].end(),
        mWordSets[b].begin(),
        mWordSets[b].end(),
        std::back_inserter(commonWords)
    );
    
    double denominator = std::log(double(mWordSizes[a])) + std::log(double(mWordSizes[b]));
    if (std::fabs(denominator) < 1e-6) {
        return 0.0;
    }
    return 1.0 * commonWords.size() / denominator;
}

bool TextRanker::CalcSentenceScores()
{
    if (mAdjacencyMatrix.empty() || mAdjacencyMatrix[0].empty() || mOutWeightSum.empty()) {
        return false;
    }

    int kDim = mSentences.size();

    // 初始时刻，所有节点的score都为1.0
    mScores.clear();
    mScores.resize(kDim, 1.0);

    // iterate
    int iterNum=0;
    for (; iterNum<mMaxIter; ++iterNum) {
        double maxDelta = 0.0;
        vector<double> newScores(kDim, 0.0); // current iteration score

        for (int i=0; i<kDim; ++i) {
            double sum_weight = 0.0;
            for (int j=0; j<kDim; ++j) {
                if (i == j || mOutWeightSum[j] < 1e-6)
                    continue;
                double weight = mAdjacencyMatrix[j][i];
                sum_weight += weight/mOutWeightSum[j] * mScores[j];
            }
            double newScore = 1.0-m_d + m_d*sum_weight;
            newScores[i] = newScore;

            double delta = fabs(newScore - mScores[i]);
            maxDelta = max(maxDelta, delta);
        }

        mScores = newScores;
        if (maxDelta < mTol) {
            break;
        }
    }

    // std::cout << "iterNum: " << iterNum << "\n";
    return true;
}

void TextRanker::StringReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) { return; }

    bool isToContainFrom = false;   // In case 'to' contains 'from', like replacing 'x' with 'yx'
    if (to.find(from, 0) != std::string::npos) {
        isToContainFrom = true;
    }

    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        if (isToContainFrom) {
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
}
