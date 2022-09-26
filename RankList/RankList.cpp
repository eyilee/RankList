#include <iostream>
#include <chrono>
#include <map>

#include <cmath>
#include <stack>
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

template<typename TID, typename TScore, int N>
class CRankList
{
public:
    using TRankNode = CRankNode<TID, TScore>;

public:
    CRankList ()
        : m_pRoot (nullptr)
    {
    }

    virtual ~CRankList ()
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
            RemoveRank (_nID);
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
            int size = std::max (m_pRoot->m_nLevel - 1, 0);
            kParents.reserve (size);

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
                    if (pParent->m_nCount >= pow (N, pParent->m_nLevel)) {
                        InsertUp (GetTopNode (pNewNode), pParent);
                    }
                }

                kParents.pop_back ();
            }

            if (CalcCount (m_pRoot) >= pow (N, m_pRoot->m_nLevel)) {
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

    // DEBUG
    void Print ()
    {
        TRankNode* pDown = m_pRoot;
        while (pDown != nullptr) {
            TRankNode* pNode = pDown;
            while (pNode != nullptr)
            {
                //std::cout << "id:" << pNode->GetID () << ", score: " << pNode->GetScore () << ", level: " << pNode->m_nLevel << std::endl;
                CheckNext (pNode);
                pNode = pNode->m_pNext;
            }
            std::cout << "level: " << pDown->m_nLevel << std::endl;
            //std::cout << "id:" << pDown->GetID () << ", score: " << pDown->GetScore () << ", level: " << pDown->m_nLevel << std::endl;
            CheckDown (pDown);
            pDown = pDown->m_pDown;
        }
    }

    void CheckNext (TRankNode* pNode)
    {
        if (pNode == nullptr || pNode->m_pNext == nullptr) {
            return;
        }

        if (pNode->GetScore () < pNode->m_pNext->GetScore ()) {
            std::cerr << "[error] id: " << pNode->GetID () << ", score: " << pNode->GetScore () << std::endl;
            std::cerr << "        id: " << pNode->m_pNext->GetID () << ", score: " << pNode->m_pNext->GetScore () << std::endl;
        }
    }

    void CheckDown (TRankNode* pNode)
    {
        if (pNode == nullptr || pNode->m_pDown == nullptr) {
            return;
        }

        if (pNode->GetScore () != pNode->m_pDown->GetScore ()) {
            std::cerr << "[error] id: " << pNode->GetID () << ", score: " << pNode->GetScore () << std::endl;
            std::cerr << "        id: " << pNode->m_pNext->GetID () << ", score: " << pNode->m_pNext->GetScore () << std::endl;
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

            delete _pNode;

            _pNode = pDown;
        }
    }

    void CreateRoot (TID _nID, TScore _nScore)
    {
        if (m_pRoot != nullptr) {
            return;
        }

        m_pRoot = new TRankNode (2, 1, _nID, _nScore);

        TRankNode* pNewNode = new TRankNode (1, 1, _nID, _nScore);
        pNewNode->m_pUp = m_pRoot;

        m_pRoot->m_pDown = pNewNode;

        SetNodeMapNode (_nID, m_pRoot);
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

        SetNodeMapNode (_nID, m_pRoot);
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

            SetNodeMapNode (m_pRoot->GetID (), m_pRoot);

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

        TRankNode* pNewNode = new TRankNode (1, 1, _nID, _nScore);
        pNewNode->m_pNext = _pNode->m_pNext;
        pNewNode->m_pPrev = _pNode;

        if (_pNode->m_pNext != nullptr) {
            _pNode->m_pNext->m_pPrev = pNewNode;
        }
        _pNode->m_pNext = pNewNode;

        SetNodeMapNode (_nID, pNewNode);

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

        TRankNode* pNewNode = new TRankNode (_pParent->m_nLevel, CalcCount (_pNode), _pNode->GetID (), _pNode->GetScore ());
        pNewNode->m_pDown = _pNode;
        pNewNode->m_pNext = _pParent->m_pNext;
        pNewNode->m_pPrev = _pParent;

        if (_pParent->m_pNext != nullptr) {
            _pParent->m_pNext->m_pPrev = pNewNode;
        }
        _pParent->m_pNext = pNewNode;
        _pParent->m_nCount -= pNewNode->m_nCount;

        _pNode->m_pUp = pNewNode;

        SetNodeMapNode (pNewNode->GetID (), pNewNode);
    }

    void IncreaseLevel ()
    {
        if (m_pRoot == nullptr) {
            return;
        }

        TRankNode* pNewNode = new TRankNode (m_pRoot->m_nLevel + 1, CalcCount (m_pRoot), m_pRoot->GetID (), m_pRoot->GetScore ());
        pNewNode->m_pDown = m_pRoot;

        m_pRoot->m_pUp = pNewNode;
        m_pRoot = pNewNode;

        SetNodeMapNode (m_pRoot->GetID (), m_pRoot);
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

    void SetNodeMapNode (TID _nID, TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return;
        }

        auto it = m_kNodeMap.find (_nID);
        if (it == m_kNodeMap.end ()) {
            m_kNodeMap.emplace (_nID, _pNode);
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

protected:
    TRankNode* m_pRoot;
    std::unordered_map<TID, TRankNode*> m_kNodeMap;

private:
};

class CMyRankList : public CRankList<int, int, 4>
{
};

int main ()
{
    srand (static_cast<unsigned int> (time (nullptr)));

    int times = 5;
    while (times--)
    {
        int size = rand () % 20000 + 10000;
        std::cout << "size: " << size << std::endl;

        auto start = std::chrono::steady_clock::now ();

        CMyRankList kRankList;
        for (int i = 1; i <= size; i++) {
            kRankList.SetRank (i, rand ());
        }

        auto diff = std::chrono::steady_clock::now () - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (diff);
        std::cout << "insert: " << ms.count () / 1000.0 << "s, ";

        start = std::chrono::steady_clock::now ();

        for (int i = 1; i <= size; i++) {
            kRankList.SetRank (i, rand ());
        }

        diff = std::chrono::steady_clock::now () - start;
        ms = std::chrono::duration_cast<std::chrono::milliseconds> (diff);
        std::cout << "update: " << ms.count () / 1000.0 << "s, ";

        start = std::chrono::steady_clock::now ();

        //kRankList.Print ();
        kRankList.CheckRank ();

        diff = std::chrono::steady_clock::now () - start;
        ms = std::chrono::duration_cast<std::chrono::milliseconds> (diff);
        std::cout << "check: " << ms.count () / 1000.0 << "s, ";

        start = std::chrono::steady_clock::now ();

        for (int i = 1; i <= size - 1; i++) {
            kRankList.RemoveRank (i);
        }

        diff = std::chrono::steady_clock::now () - start;
        ms = std::chrono::duration_cast<std::chrono::milliseconds> (diff);
        std::cout << "remove: " << ms.count () / 1000.0 << "s" << std::endl;

        kRankList.Print ();
        kRankList.CheckRank ();
    }

    return 0;
}