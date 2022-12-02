#include <iostream>
#include <chrono>

#include <cmath>
#include <unordered_map>
#include <vector>

template<typename TID, typename TScore>
class CRankNode
{
public:
    CRankNode (int _nLevel, int _nCount, TID _nID, TScore _nScore)
        : m_nLevel (_nLevel)
        , m_nCount (_nCount)
        , m_nID (_nID)
        , m_nScore (_nScore)
        , m_pUp (nullptr)
        , m_pDown (nullptr)
        , m_pPrev (nullptr)
        , m_pNext (nullptr)
    {
    }

    virtual ~CRankNode ()
    {
    }

    inline void SetID (TID _nID) { m_nID = _nID; }
    inline void SetScore (int _nScore) { m_nScore = _nScore; }
    inline TID GetID () const { return m_nID; }
    inline int GetScore () const { return m_nScore; }

    int m_nLevel;
    int m_nCount;
    TID m_nID;
    TScore m_nScore;

    CRankNode* m_pUp;
    CRankNode* m_pDown;
    CRankNode* m_pPrev;
    CRankNode* m_pNext;
};

template<typename TID, typename TScore, int N = 4>
class CRankList
{
public:
    using TRankNode = CRankNode<TID, TScore>;

public:
    CRankList ()
        : m_pRoot (nullptr)
    {
    }

    CRankList (const CRankList&) = delete;

    virtual ~CRankList ()
    {
        Clear ();
    }

    TScore GetScore (TID _nID)
    {
        TRankNode* pNodeMapNode = GetNodeMapNode (_nID);
        if (pNodeMapNode == nullptr) {
            return 0;
        }

        return pNodeMapNode->GetScore ();
    }

    int GetRank (TID _nID)
    {
        TRankNode* pNodeMapNode = GetNodeMapNode (_nID);
        if (pNodeMapNode == nullptr) {
            return 0;
        }

        return CalcRank (pNodeMapNode) + 1;
    }

    void SetRank (TID _nID, TScore _nScore)
    {
        TRankNode* pNodeMapNode = GetNodeMapNode (_nID);
        if (pNodeMapNode != nullptr && pNodeMapNode->GetScore () == _nScore) {
            return;
        }

        if (pNodeMapNode != nullptr) {
            pNodeMapNode = GetBottomNode (pNodeMapNode);

            bool bIsChange = false;
            if (pNodeMapNode->m_pPrev != nullptr) {
                bIsChange |= _nScore >= pNodeMapNode->m_pPrev->GetScore ();
            }

            if (pNodeMapNode->m_pNext != nullptr) {
                bIsChange |= _nScore < pNodeMapNode->m_pNext->GetScore ();
            }

            if (bIsChange) {
                RemoveRank (_nID);
            }
            else {
                while (pNodeMapNode != nullptr) {
                    pNodeMapNode->SetScore (_nScore);
                    pNodeMapNode = pNodeMapNode->m_pUp;
                }
                return;
            }
        }

        if (m_pRoot == nullptr) {
            CreateRoot (_nID, _nScore);
        }
        else if (m_pRoot != nullptr && m_pRoot->GetScore () < _nScore) {
            InsertRoot (_nID, _nScore);
        }
        else
        {
            std::vector<TRankNode*> kParents;
            int nSize = std::max (m_pRoot->m_nLevel - 1, 0);
            kParents.reserve (nSize);

            TRankNode* pNode = FindNode (_nScore, m_pRoot, kParents);
            if (pNode == nullptr) {
                return;
            }

            TRankNode* pNewNode = InsertNext (pNode, _nID, _nScore);
            if (pNewNode == nullptr) {
                return;
            }

            while (!kParents.empty ())
            {
                TRankNode* pParent = kParents.back ();

                pParent->m_nCount++;

                if (pParent->m_pDown != nullptr)
                {
                    if (pParent->m_nCount > pow (N, pParent->m_nLevel - 1)) {
                        InsertUp (GetTopNode (pNewNode), pParent);
                    }
                }

                kParents.pop_back ();
            }

            if (CalcCount (m_pRoot) > pow (N, m_pRoot->m_nLevel)) {
                IncreaseLevel ();
            }
        }
    }

    void RemoveRank (TID _nID)
    {
        if (m_pRoot != nullptr && m_pRoot->GetID () == _nID) {
            RemoveRoot ();
        }
        else {
            RemoveNode (GetNodeMapNode (_nID));
        }

        RemoveNodeMapNode (_nID);
    }

    TRankNode* QueryRank (int _nRank)
    {
        int nMaxSize = CalcCount (m_pRoot);
        if (_nRank < 1 || _nRank > nMaxSize) {
            return nullptr;
        }

        int nCount = 0;

        TRankNode* pNode = m_pRoot;
        while (pNode != nullptr)
        {
            while (pNode->m_pNext != nullptr)
            {
                if (nCount + pNode->m_nCount >= _nRank) {
                    break;
                }

                nCount += pNode->m_nCount;
                pNode = pNode->m_pNext;
            }

            if (pNode->m_pDown == nullptr) {
                return pNode;
            }

            pNode = pNode->m_pDown;
        }

        return pNode;
    }

    void QueryRanks (int _nRank, int _nSize, std::vector<TRankNode*>& _kRankNodes)
    {
        _kRankNodes.clear ();

        int nMaxSize = CalcCount (m_pRoot);
        if (_nRank < 1 || _nRank > nMaxSize) {
            return;
        }

        int nCount = std::min (_nSize, nMaxSize - _nRank + 1);
        _kRankNodes.reserve (nCount);

        TRankNode* pNode = GetBottomNode (QueryRank (_nRank));
        while (pNode != nullptr && nCount > 0) {
            _kRankNodes.emplace_back (pNode);
            pNode = pNode->m_pNext;
            nCount--;
        }
    }

    void GetRankList (std::vector<std::pair<TID, TScore>>& _kRankList)
    {
        _kRankList.clear ();

        int nCount = CalcCount (m_pRoot);
        if (nCount < 1) {
            return;
        }

        _kRankList.reserve (nCount);

        TRankNode* pNode = GetBottomNode (m_pRoot);
        while (pNode != nullptr) {
            _kRankList.emplace_back (pNode->GetID (), pNode->GetScore ());
            pNode = pNode->m_pNext;
        }
    }

    void GetRankList (int _nRank, int _nSize, std::vector<std::pair<TID, TScore>>& _kRankList)
    {
        _kRankList.clear ();

        int nMaxSize = CalcCount (m_pRoot);
        if (_nRank < 1 || _nRank > nMaxSize) {
            return;
        }

        int nCount = std::min (_nSize, nMaxSize - _nRank + 1);
        _kRankList.reserve (nCount);

        TRankNode* pNode = GetBottomNode (QueryRank (_nRank));
        while (pNode != nullptr && nCount > 0) {
            _kRankList.emplace_back (pNode->GetID (), pNode->GetScore ());
            pNode = pNode->m_pNext;
            nCount--;
        }
    }

    void Clear ()
    {
        ClearList ();
        ClearPool ();
    }

    void Swap (CRankList& _kRankList)
    {
        TRankNode* pNode = m_pRoot;
        m_pRoot = _kRankList.m_pRoot;
        _kRankList.m_pRoot = pNode;

        m_kNodeMap.swap (_kRankList.m_kNodeMap);
        m_kPool.swap (_kRankList.m_kPool);
    }

    // DEBUG
    void Print ()
    {
        TRankNode* pNode = m_pRoot;
        while (pNode != nullptr)
        {
            TRankNode* pDown = pNode->m_pDown;
            while (pNode != nullptr)
            {
                TRankNode* pNext = pNode->m_pNext;
                std::cout << "level: " << pNode->m_nLevel << ", count: " << pNode->m_nCount << ", id:" << pNode->GetID () << ", score: " << pNode->GetScore () << std::endl;
                pNode = pNext;
            }
            pNode = pDown;
        }
    }

    void CheckScore ()
    {
        TRankNode* pNode = m_pRoot;
        while (pNode != nullptr)
        {
            TRankNode* pDown = pNode->m_pDown;
            CheckDown (pDown);
            while (pNode != nullptr)
            {
                TRankNode* pNext = pNode->m_pNext;
                CheckNext (pNext);
                pNode = pNext;
            }
            pNode = pDown;
        }
    }

    void CheckNext (TRankNode* pNode)
    {
        if (pNode == nullptr || pNode->m_pNext == nullptr) {
            return;
        }

        if (pNode->GetScore () < pNode->m_pNext->GetScore ()) {
            std::cout << "id: " << pNode->GetID () << ", score: " << pNode->GetScore () << std::endl;
            std::cout << "next id: " << pNode->m_pNext->GetID () << ", score: " << pNode->m_pNext->GetScore () << std::endl;
        }
    }

    void CheckDown (TRankNode* pNode)
    {
        if (pNode == nullptr || pNode->m_pDown == nullptr) {
            return;
        }

        if (pNode->GetScore () != pNode->m_pDown->GetScore ()) {
            std::cout << "id: " << pNode->GetID () << ", score: " << pNode->GetScore () << std::endl;
            std::cout << "down id: " << pNode->m_pDown->GetID () << ", score: " << pNode->m_pDown->GetScore () << std::endl;
        }
    }

    void CheckRank ()
    {
        TRankNode* pNode = m_pRoot;
        if (pNode == nullptr) {
            return;
        }

        while (pNode->m_pDown != nullptr) {
            pNode = pNode->m_pDown;
        }

        int nRank = 0;
        while (pNode != nullptr) {
            nRank++;

            TRankNode* pResult = QueryRank (nRank);
            if (pNode != pResult) {
                std::cout << "id: " << pNode->GetID () << "(" << (pResult == nullptr ? 0 : pResult->GetID ()) << ")" << ", rank: " << nRank << std::endl;
            }

            if (nRank != GetRank (pNode->GetID ())) {
                std::cout << "id: " << pNode->GetID () << ", rank: " << nRank << "(" << GetRank (pNode->GetID ()) << ")" << std::endl;
            }

            pNode = pNode->m_pNext;
        }
    }

private:
    TRankNode* GetTopNode (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return nullptr;
        }

        while (_pNode->m_pUp != nullptr) {
            _pNode = _pNode->m_pUp;
        }

        return _pNode;
    }

    TRankNode* GetBottomNode (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return nullptr;
        }

        while (_pNode->m_pDown != nullptr) {
            _pNode = _pNode->m_pDown;
        }

        return _pNode;
    }

    TRankNode* FindNode (int _nScore, TRankNode* _pNode, std::vector<TRankNode*>& _kParents)
    {
        if (_pNode == nullptr) {
            return nullptr;
        }

        while (_pNode != nullptr)
        {
            while (_pNode->m_pNext != nullptr)
            {
                if (_nScore > _pNode->m_pNext->GetScore ()) {
                    break;
                }

                _pNode = _pNode->m_pNext;
            }

            if (_pNode->m_pDown == nullptr) {
                return _pNode;
            }

            _kParents.emplace_back (_pNode);
            _pNode = _pNode->m_pDown;
        }

        return _pNode;
    }

    void RemoveNode (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return;
        }

        TRankNode* pParent = _pNode;
        while (pParent != nullptr)
        {
            while (pParent->m_pUp != nullptr)
            {
                pParent = pParent->m_pUp;
                pParent->m_nCount--;
            }

            pParent = pParent->m_pPrev;
        }

        while (_pNode != nullptr)
        {
            TRankNode* pDown = _pNode->m_pDown;

            if (_pNode->m_nLevel > 1)
            {
                if (_pNode->m_pPrev != nullptr) {
                    _pNode->m_pPrev->m_nCount += _pNode->m_nCount - 1;
                }
            }

            if (_pNode->m_pPrev != nullptr) {
                _pNode->m_pPrev->m_pNext = _pNode->m_pNext;
            }

            if (_pNode->m_pNext != nullptr) {
                _pNode->m_pNext->m_pPrev = _pNode->m_pPrev;
            }

            PushNode (_pNode);

            _pNode = pDown;
        }
    }

    void CreateRoot (TID _nID, TScore _nScore)
    {
        if (m_pRoot != nullptr) {
            return;
        }
                
        m_pRoot = PopNode (2, 1, _nID, _nScore);

        TRankNode* pNewNode = PopNode (1, 1, _nID, _nScore);
        pNewNode->m_pUp = m_pRoot;

        m_pRoot->m_pDown = pNewNode;

        SetNodeMapNode (m_pRoot);
    }

    void InsertRoot (TID _nID, TScore _nScore)
    {
        if (m_pRoot == nullptr) {
            return;
        }

        TRankNode* pNewNode = InsertNext (GetBottomNode (m_pRoot), m_pRoot->GetID (), m_pRoot->GetScore ());
        if (pNewNode == nullptr) {
            return;
        }

        TRankNode* pNode = m_pRoot;
        while (pNode != nullptr)
        {
            pNode->SetID (_nID);
            pNode->SetScore (_nScore);

            if (pNode->m_pDown != nullptr) {
                pNode->m_nCount++;
            }

            pNode = pNode->m_pDown;
        }

        SetNodeMapNode (m_pRoot);
    }

    void RemoveRoot ()
    {
        if (m_pRoot == nullptr) {
            return;
        }

        TRankNode* pBottom = GetBottomNode (m_pRoot);
        if (pBottom == nullptr) {
            return;
        }

        TRankNode* pNext = pBottom->m_pNext;
        if (pNext != nullptr)
        {
            TID nID = pNext->GetID ();
            TScore nScore = pNext->GetScore ();

            TRankNode* pNode = m_pRoot;
            while (pNode != nullptr)
            {
                pNode->SetID (nID);
                pNode->SetScore (nScore);
                pNode = pNode->m_pDown;
            }

            SetNodeMapNode (m_pRoot);

            RemoveNode (GetTopNode (pNext));
        }
        else
        {
            RemoveNode (m_pRoot);

            m_pRoot = nullptr;
        }
    }

    TRankNode* InsertNext (TRankNode* _pNode, TID _nID, TScore _nScore)
    {
        if (_pNode == nullptr) {
            return nullptr;
        }

        if (_pNode->m_nLevel != 1) {
            return nullptr;
        }

        TRankNode* pNewNode = PopNode (1, 1, _nID, _nScore);
        pNewNode->m_pNext = _pNode->m_pNext;
        pNewNode->m_pPrev = _pNode;

        if (_pNode->m_pNext != nullptr) {
            _pNode->m_pNext->m_pPrev = pNewNode;
        }
        _pNode->m_pNext = pNewNode;

        SetNodeMapNode (pNewNode);

        return pNewNode;
    }

    void InsertUp (TRankNode* _pNode, TRankNode* _pParent)
    {
        if (_pNode == nullptr || _pParent == nullptr) {
            return;
        }

        if (_pNode->m_nLevel + 1 != _pParent->m_nLevel) {
            return;
        }

        TRankNode* pNewNode = PopNode (_pParent->m_nLevel, CalcCount (_pNode), _pNode->GetID (), _pNode->GetScore ());
        pNewNode->m_pDown = _pNode;
        pNewNode->m_pNext = _pParent->m_pNext;
        pNewNode->m_pPrev = _pParent;

        if (_pParent->m_pNext != nullptr) {
            _pParent->m_pNext->m_pPrev = pNewNode;
        }
        _pParent->m_pNext = pNewNode;
        _pParent->m_nCount -= pNewNode->m_nCount;

        _pNode->m_pUp = pNewNode;

        SetNodeMapNode (pNewNode);
    }

    void IncreaseLevel ()
    {
        if (m_pRoot == nullptr) {
            return;
        }

        TRankNode* pNewNode = PopNode (m_pRoot->m_nLevel + 1, CalcCount (m_pRoot), m_pRoot->GetID (), m_pRoot->GetScore ());
        pNewNode->m_pDown = m_pRoot;

        m_pRoot->m_pUp = pNewNode;
        m_pRoot = pNewNode;

        SetNodeMapNode (m_pRoot);
    }

    int CalcRank (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return 0;
        }

        int nCount = 0;

        while (_pNode->m_pUp != nullptr) {
            _pNode = _pNode->m_pUp;
        }

        while (_pNode->m_pPrev != nullptr)
        {
            _pNode = _pNode->m_pPrev;

            nCount += _pNode->m_nCount;

            while (_pNode->m_pUp != nullptr) {
                _pNode = _pNode->m_pUp;
            }
        }

        return nCount;
    }

    int CalcCount (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return 0;
        }

        int nCount = _pNode->m_nCount;

        while (_pNode->m_pNext != nullptr)
        {
            _pNode = _pNode->m_pNext;

            if (_pNode->m_pUp != nullptr) {
                break;
            }

            nCount += _pNode->m_nCount;
        }

        return nCount;
    }

    TRankNode* GetNodeMapNode (TID _nID)
    {
        auto it = m_kNodeMap.find (_nID);
        if (it == m_kNodeMap.end ()) {
            return nullptr;
        }
        return it->second;
    }

    void SetNodeMapNode (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return;
        }

        TID nID = _pNode->GetID ();

        auto it = m_kNodeMap.find (nID);
        if (it == m_kNodeMap.end ()) {
            m_kNodeMap.emplace (nID, _pNode);
        }
        else {
            it->second = _pNode;
        }
    }

    void RemoveNodeMapNode (TID _nID)
    {
        auto it = m_kNodeMap.find (_nID);
        if (it == m_kNodeMap.end ()) {
            return;
        }
        else {
            it->second = nullptr;
        }
    }

    TRankNode* PopNode (int _nLevel, int _nCount, TID _nID, TScore _nScore)
    {
        TRankNode* pNode = nullptr;
        if (!m_kPool.empty ())
        {
            pNode = m_kPool.back ();

            pNode->m_nLevel = _nLevel;
            pNode->m_nCount = _nCount;
            pNode->SetID (_nID);
            pNode->SetScore (_nScore);

            m_kPool.pop_back ();
        }

        if (pNode == nullptr) {
            pNode = new TRankNode (_nLevel, _nCount, _nID, _nScore);
        }

        return pNode;
    }

    void PushNode (TRankNode*& _pNode)
    {
        if (_pNode == nullptr) {
            return;
        }

        _pNode->m_pUp = nullptr;
        _pNode->m_pDown = nullptr;
        _pNode->m_pPrev = nullptr;
        _pNode->m_pNext = nullptr;

        m_kPool.emplace_back (_pNode);

        _pNode = nullptr;
    }

    void ClearList ()
    {
        TRankNode* pNode = m_pRoot;
        while (pNode != nullptr)
        {
            TRankNode* pDown = pNode->m_pDown;
            while (pNode != nullptr)
            {
                TRankNode* pNext = pNode->m_pNext;
                delete pNode;
                pNode = pNext;
            }
            pNode = pDown;
        }

        m_pRoot = nullptr;
        m_kNodeMap.clear ();
    }

    void ClearPool ()
    {
        for (auto& pNode : m_kPool) {
            if (pNode != nullptr) {
                delete pNode;
            }
        }

        m_kPool.clear ();
        m_kPool.shrink_to_fit ();
    }

protected:
    TRankNode* m_pRoot;
    std::unordered_map<TID, TRankNode*> m_kNodeMap;

private:
    std::vector<TRankNode*> m_kPool;
};

template<int N>
void test ()
{
    using TRankList = CRankList<int, int, N>;

    int size = 80000;
    std::cout << "size: " << size << ", N: " << N << std::endl;

    long long insert = 0;
    long long update = 0;
    long long check = 0;
    long long remove = 0;
    TRankList kRankList;

    int T = 1;
    int times = T;
    while (times--)
    {
        kRankList.Clear ();

        {
            auto start = std::chrono::steady_clock::now ();

            for (int i = 1; i <= size; i++) {
                kRankList.SetRank (i, rand ());
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            insert += ms.count ();
        }

        {
            auto start = std::chrono::steady_clock::now ();

            for (int i = 0; i < size; i++) {
                if (rand () % 2 == 0) {
                    kRankList.SetRank (rand () % size, rand ());
                }
                else {
                    kRankList.RemoveRank (rand () % size);
                }
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            update += ms.count ();
        }

        {
            auto start = std::chrono::steady_clock::now ();

            kRankList.CheckScore ();
            kRankList.CheckRank ();

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            check += ms.count ();
        }

        {
            auto start = std::chrono::steady_clock::now ();

            for (int i = 1; i <= size; i++) {
                kRankList.RemoveRank (i);
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            remove += ms.count ();
        }
    }

    std::cout << "insert: " << insert / 1000.0 / T << "s, ";
    std::cout << "update: " << update / 1000.0 / T << "s, ";
    std::cout << "check: " << check / 1000.0 / T << "s, ";
    std::cout << "remove: " << remove / 1000.0 / T << "s" << std::endl;
}

int main ()
{
    srand (static_cast<unsigned int> (time (nullptr)));

    //test<2> ();
    test<4> ();
    //test<6> ();
    //test<8> ();
    //test<10> ();
    //test<12> ();
    //test<14> ();
    //test<16> ();
    //test<18> ();
    //test<20> ();
    //test<22> ();
    //test<24> ();
    //test<26> ();
    //test<28> ();
    //test<30> ();
    //test<32> ();
    //test<34> ();
    //test<36> ();
    //test<38> ();
    //test<40> ();

    return 0;
}