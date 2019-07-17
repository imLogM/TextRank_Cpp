#ifndef TEXT_RANKER_H
#define TEXT_RANKER_H

#include <string>
#include <vector>
#include <set>

class TextRanker {
public:
    explicit TextRanker() 
        : m_d(0.85), mMaxIter(100), mTol(1.0e-5) { }
    explicit TextRanker(double d, int maxIter, double tol)
        : m_d(d), mMaxIter(maxIter), mTol(tol) { }

     ~TextRanker() { }

    bool ExtractKeySentences(const std::string& input, std::vector<std::string>& outputs, int topK);

private:
    bool ExtractSentences(const std::string& input, std::vector<std::string>& output);
    bool RemoveDuplicates(const std::vector<std::string>& input, std::vector<std::string>& output);
    bool BuildGraph(const std::vector<std::string>& sentences);
    double GetSimilarity(int a, int b);
    bool CalcSentenceScores();
    bool InitWordSet(const std::vector<std::string>& sentences);
    void StringReplaceAll(std::string& str, const std::string& from, const std::string& to);

private:
    double m_d;  // 迭代公式中的参数d
    int mMaxIter;   // 迭代的最大轮数
    double mTol;   // 迭代精度
    std::vector<std::string> mSentences;  // 切分后的句子
    std::vector< std::vector<double> > mAdjacencyMatrix;  // 邻接矩阵
    std::vector<double> mOutWeightSum;  // 每个节点的出链的权重和
    std::vector<double> mScores;  // 每个节点的score
    std::vector< std::set<std::string> > mWordSets;    // 每个句子包含的词的集合
    std::vector<int> mWordSizes;  // 每个句子包含的词的数量
};

#endif
