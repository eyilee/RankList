#include <iostream>

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
        , m_pkUp (nullptr)
        , m_pkDown (nullptr)
        , m_pkPrev (nullptr)
        , m_pkNext (nullptr)
    {
    }

    virtual ~CRankNode ()
    {
    }

    int m_nLevel;
    int m_nCount;
    TID m_nID;
    TScore m_nScore;

    CRankNode* m_pkUp;
    CRankNode* m_pkDown;
    CRankNode* m_pkPrev;
    CRankNode* m_pkNext;
};

template<typename TID, typename TScore, int N = 4>
class CRankList
{
public:
    using TRankNode = CRankNode<TID, TScore>;

public:
    CRankList ()
        : m_pkRoot (nullptr)
    {
    }

    CRankList (const CRankList&) = delete;

    virtual ~CRankList ()
    {
        Clear ();
    }

    TScore GetScore (TID _nID)
    {
        TRankNode* mapNode = GetMapNode (_nID);
        if (mapNode == nullptr) {
            return 0;
        }

        return mapNode->m_nScore;
    }

    int GetRank (TID _nID)
    {
        TRankNode* mapNode = GetMapNode (_nID);
        if (mapNode == nullptr) {
            return 0;
        }

        return CalcRank (mapNode) + 1;
    }

    void SetRank (TID _nID, TScore _nScore)
    {
        TRankNode* mapNode = GetMapNode (_nID);
        if (mapNode != nullptr && mapNode->m_nScore == _nScore) {
            return;
        }

        if (mapNode != nullptr) {
            mapNode = GetBottomNode (mapNode);

            bool hasChanged = false;
            if (mapNode->m_pkPrev != nullptr) {
                hasChanged |= _nScore >= mapNode->m_pkPrev->m_nScore;
            }

            if (mapNode->m_pkNext != nullptr) {
                hasChanged |= _nScore < mapNode->m_pkNext->m_nScore;
            }

            if (hasChanged) {
                RemoveRank (_nID);
            }
            else
            {
                while (mapNode != nullptr)
                {
                    mapNode->m_nScore = _nScore;
                    mapNode = mapNode->m_pkUp;
                }
                return;
            }
        }

        if (m_pkRoot == nullptr) {
            CreateRoot (_nID, _nScore);
        }
        else if (m_pkRoot != nullptr && m_pkRoot->m_nScore < _nScore) {
            InsertRoot (_nID, _nScore);
        }
        else
        {
            std::vector<TRankNode*> parents;
            int size = std::max (m_pkRoot->m_nLevel - 1, 0);
            parents.reserve (size);

            TRankNode* node = FindPrevNode (_nScore, m_pkRoot, parents);
            if (node == nullptr) {
                return;
            }

            TRankNode* newNode = InsertNext (node, _nID, _nScore);
            if (newNode == nullptr) {
                return;
            }

            TRankNode* topNode = newNode;
            for (auto it = parents.rbegin (); it != parents.rend (); it++)
            {
                TRankNode* parent = *it;
                if (parent == nullptr) {
                    break;
                }

                parent->m_nCount++;

                if (parent->m_pkDown != nullptr)
                {
                    if (parent->m_nCount > pow (N, parent->m_nLevel - 1)) {
                        topNode = InsertUp (topNode, parent);
                    }
                }
            }

            if (CalcCount (m_pkRoot) > pow (N, m_pkRoot->m_nLevel)) {
                IncreaseLevel ();
            }
        }
    }

    void RemoveRank (TID _nID)
    {
        if (m_pkRoot != nullptr && m_pkRoot->m_nID == _nID) {
            RemoveRoot ();
        }
        else {
            RemoveNode (GetMapNode (_nID));
        }

        RemoveMapNode (_nID);
    }

    TRankNode* QueryRank (int _nRank)
    {
        int maxSize = CalcCount (m_pkRoot);
        if (_nRank < 1 || _nRank > maxSize) {
            return nullptr;
        }

        int count = 0;

        TRankNode* node = m_pkRoot;
        while (node != nullptr)
        {
            while (node->m_pkNext != nullptr)
            {
                if (count + node->m_nCount >= _nRank) {
                    break;
                }

                count += node->m_nCount;
                node = node->m_pkNext;
            }

            if (node->m_pkDown == nullptr) {
                return node;
            }

            node = node->m_pkDown;
        }

        return node;
    }

    void QueryRanks (int _nRank, int _nSize, std::vector<TRankNode*>& _rkRankNodes)
    {
        _rkRankNodes.clear ();

        int maxSize = CalcCount (m_pkRoot);
        if (_nRank < 1 || _nRank > maxSize) {
            return;
        }

        int count = std::min (_nSize, maxSize - _nRank + 1);
        _rkRankNodes.reserve (count);

        TRankNode* node = GetBottomNode (QueryRank (_nRank));
        while (node != nullptr && count > 0)
        {
            _rkRankNodes.emplace_back (node);
            node = node->m_pkNext;
            count--;
        }
    }

    void GetRankList (std::vector<std::pair<TID, TScore>>& _rkRankList)
    {
        _rkRankList.clear ();

        int count = CalcCount (m_pkRoot);
        if (count < 1) {
            return;
        }

        _rkRankList.reserve (count);

        TRankNode* node = GetBottomNode (m_pkRoot);
        while (node != nullptr)
        {
            _rkRankList.emplace_back (node->m_nID, node->m_nScore);
            node = node->m_pkNext;
        }
    }

    void GetRankList (int _nRank, int _nSize, std::vector<std::pair<TID, TScore>>& _rkRankList)
    {
        _rkRankList.clear ();

        int maxSize = CalcCount (m_pkRoot);
        if (_nRank < 1 || _nRank > maxSize) {
            return;
        }

        int count = std::min (_nSize, maxSize - _nRank + 1);
        _rkRankList.reserve (count);

        TRankNode* node = GetBottomNode (QueryRank (_nRank));
        while (node != nullptr && count > 0)
        {
            _rkRankList.emplace_back (node->m_nID, node->m_nScore);
            node = node->m_pkNext;
            count--;
        }
    }

    void Clear ()
    {
        ClearList ();
        ClearPool ();
    }

    void Swap (CRankList& _rkRankList)
    {
        std::swap (m_pkRoot, _rkRankList.m_pkRoot);

        m_kNodeMap.swap (_rkRankList.m_kNodeMap);
        m_kPool.swap (_rkRankList.m_kPool);
    }

    // DEBUG
    void Print ()
    {
        TRankNode* node = m_pkRoot;
        while (node != nullptr)
        {
            TRankNode* down = node->m_pkDown;
            while (node != nullptr)
            {
                TRankNode* next = node->m_pkNext;
                std::cout << "level: " << node->m_nLevel << ", count: " << node->m_nCount << ", id:" << node->m_nID << ", score: " << node->m_nScore << std::endl;
                node = next;
            }
            node = down;
        }
    }

    void CheckScore ()
    {
        TRankNode* node = m_pkRoot;
        while (node != nullptr)
        {
            TRankNode* down = node->m_pkDown;
            CheckDown (down);
            while (node != nullptr)
            {
                TRankNode* next = node->m_pkNext;
                CheckNext (next);
                node = next;
            }
            node = down;
        }
    }

    void CheckNext (TRankNode* _pkNode)
    {
        if (_pkNode == nullptr || _pkNode->m_pkNext == nullptr) {
            return;
        }

        if (_pkNode->m_nScore < _pkNode->m_pkNext->m_nScore)
        {
            std::cout << "node id: " << _pkNode->m_nID << ", score: " << _pkNode->m_nScore << std::endl;
            std::cout << "next id: " << _pkNode->m_pkNext->m_nID << ", score: " << _pkNode->m_pkNext->m_nScore << std::endl;
        }
    }

    void CheckDown (TRankNode* _pkNode)
    {
        if (_pkNode == nullptr || _pkNode->m_pkDown == nullptr) {
            return;
        }

        if (_pkNode->m_nScore != _pkNode->m_pkDown->m_nScore)
        {
            std::cout << "node id: " << _pkNode->m_nID << ", score: " << _pkNode->m_nScore << std::endl;
            std::cout << "down id: " << _pkNode->m_pkDown->m_nID << ", score: " << _pkNode->m_pkDown->m_nScore << std::endl;
        }
    }

    void CheckRank ()
    {
        TRankNode* node = m_pkRoot;
        if (node == nullptr) {
            return;
        }

        while (node->m_pkDown != nullptr) {
            node = node->m_pkDown;
        }

        int rank = 0;
        while (node != nullptr)
        {
            rank++;

            TRankNode* result = QueryRank (rank);
            if (node != result) {
                std::cout << "id: " << node->m_nID << "(" << (result == nullptr ? 0 : result->m_nID) << ")" << ", rank: " << rank << std::endl;
            }

            if (rank != GetRank (node->m_nID)) {
                std::cout << "id: " << node->m_nID << ", rank: " << rank << "(" << GetRank (node->m_nID) << ")" << std::endl;
            }

            node = node->m_pkNext;
        }
    }

    int GetMaxLevel ()
    {
        return m_pkRoot == nullptr ? 0 : m_pkRoot->m_nLevel;
    }

private:
    TRankNode* GetTopNode (TRankNode* _pkNode)
    {
        if (_pkNode == nullptr) {
            return nullptr;
        }

        while (_pkNode->m_pkUp != nullptr) {
            _pkNode = _pkNode->m_pkUp;
        }

        return _pkNode;
    }

    TRankNode* GetBottomNode (TRankNode* _pkNode)
    {
        if (_pkNode == nullptr) {
            return nullptr;
        }

        while (_pkNode->m_pkDown != nullptr) {
            _pkNode = _pkNode->m_pkDown;
        }

        return _pkNode;
    }

    TRankNode* FindPrevNode (TScore _nScore, TRankNode* _pkNode, std::vector<TRankNode*>& _rkParents)
    {
        if (_pkNode == nullptr) {
            return nullptr;
        }

        while (_pkNode != nullptr)
        {
            while (_pkNode->m_pkNext != nullptr)
            {
                if (_nScore > _pkNode->m_pkNext->m_nScore) {
                    break;
                }

                _pkNode = _pkNode->m_pkNext;
            }

            if (_pkNode->m_pkDown == nullptr) {
                return _pkNode;
            }

            _rkParents.emplace_back (_pkNode);
            _pkNode = _pkNode->m_pkDown;
        }

        return _pkNode;
    }

    void RemoveNode (TRankNode* _pkNode)
    {
        _pkNode = GetTopNode (_pkNode);
        if (_pkNode == nullptr) {
            return;
        }

        TRankNode* parent = _pkNode;
        while (parent != nullptr)
        {
            while (parent->m_pkUp != nullptr)
            {
                parent = parent->m_pkUp;
                parent->m_nCount--;
            }

            parent = parent->m_pkPrev;
        }

        while (_pkNode != nullptr)
        {
            TRankNode* down = _pkNode->m_pkDown;

            if (_pkNode->m_nLevel > 1)
            {
                if (_pkNode->m_pkPrev != nullptr) {
                    _pkNode->m_pkPrev->m_nCount += _pkNode->m_nCount - 1;
                }
            }

            if (_pkNode->m_pkPrev != nullptr) {
                _pkNode->m_pkPrev->m_pkNext = _pkNode->m_pkNext;
            }

            if (_pkNode->m_pkNext != nullptr) {
                _pkNode->m_pkNext->m_pkPrev = _pkNode->m_pkPrev;
            }

            PushNode (_pkNode);

            _pkNode = down;
        }
    }

    void CreateRoot (TID _nID, TScore _nScore)
    {
        if (m_pkRoot != nullptr) {
            return;
        }

        m_pkRoot = PopNode (2, 1, _nID, _nScore);

        TRankNode* newNode = PopNode (1, 1, _nID, _nScore);
        newNode->m_pkUp = m_pkRoot;

        m_pkRoot->m_pkDown = newNode;

        SetMapNode (m_pkRoot);
    }

    void InsertRoot (TID _nID, TScore _nScore)
    {
        if (m_pkRoot == nullptr) {
            return;
        }

        TRankNode* newNode = InsertNext (GetBottomNode (m_pkRoot), m_pkRoot->m_nID, m_pkRoot->m_nScore);
        if (newNode == nullptr) {
            return;
        }

        TRankNode* node = m_pkRoot;
        while (node != nullptr)
        {
            node->m_nID = _nID;
            node->m_nScore = _nScore;

            if (node->m_pkDown != nullptr) {
                node->m_nCount++;
            }

            node = node->m_pkDown;
        }

        SetMapNode (m_pkRoot);
    }

    void RemoveRoot ()
    {
        if (m_pkRoot == nullptr) {
            return;
        }

        TRankNode* bottom = GetBottomNode (m_pkRoot);
        if (bottom == nullptr) {
            return;
        }

        TRankNode* next = bottom->m_pkNext;
        if (next != nullptr)
        {
            TID id = next->m_nID;
            TScore score = next->m_nScore;

            TRankNode* node = m_pkRoot;
            while (node != nullptr)
            {
                node->m_nID = id;
                node->m_nScore = score;
                node = node->m_pkDown;
            }

            SetMapNode (m_pkRoot);

            RemoveNode (next);
        }
        else
        {
            RemoveNode (m_pkRoot);

            m_pkRoot = nullptr;
        }
    }

    TRankNode* InsertNext (TRankNode* _pkNode, TID _nID, TScore _nScore)
    {
        if (_pkNode == nullptr) {
            return nullptr;
        }

        if (_pkNode->m_nLevel != 1) {
            return nullptr;
        }

        TRankNode* newNode = PopNode (1, 1, _nID, _nScore);
        newNode->m_pkNext = _pkNode->m_pkNext;
        newNode->m_pkPrev = _pkNode;

        if (_pkNode->m_pkNext != nullptr) {
            _pkNode->m_pkNext->m_pkPrev = newNode;
        }
        _pkNode->m_pkNext = newNode;

        SetMapNode (newNode);

        return newNode;
    }

    TRankNode* InsertUp (TRankNode* _pkNode, TRankNode* _pkParent)
    {
        if (_pkNode == nullptr || _pkParent == nullptr) {
            return nullptr;
        }

        if (_pkNode->m_nLevel + 1 != _pkParent->m_nLevel) {
            return nullptr;
        }

        TRankNode* newNode = PopNode (_pkParent->m_nLevel, CalcCount (_pkNode), _pkNode->m_nID, _pkNode->m_nScore);
        newNode->m_pkDown = _pkNode;
        newNode->m_pkNext = _pkParent->m_pkNext;
        newNode->m_pkPrev = _pkParent;

        if (_pkParent->m_pkNext != nullptr) {
            _pkParent->m_pkNext->m_pkPrev = newNode;
        }
        _pkParent->m_pkNext = newNode;
        _pkParent->m_nCount -= newNode->m_nCount;

        _pkNode->m_pkUp = newNode;

        SetMapNode (newNode);

        return newNode;
    }

    void IncreaseLevel ()
    {
        if (m_pkRoot == nullptr) {
            return;
        }

        TRankNode* newNode = PopNode (m_pkRoot->m_nLevel + 1, CalcCount (m_pkRoot), m_pkRoot->m_nID, m_pkRoot->m_nScore);
        newNode->m_pkDown = m_pkRoot;

        m_pkRoot->m_pkUp = newNode;
        m_pkRoot = newNode;

        SetMapNode (m_pkRoot);
    }

    int CalcRank (TRankNode* _pkNode)
    {
        if (_pkNode == nullptr) {
            return 0;
        }

        int count = 0;

        while (_pkNode->m_pkUp != nullptr) {
            _pkNode = _pkNode->m_pkUp;
        }

        while (_pkNode->m_pkPrev != nullptr)
        {
            _pkNode = _pkNode->m_pkPrev;

            count += _pkNode->m_nCount;

            while (_pkNode->m_pkUp != nullptr) {
                _pkNode = _pkNode->m_pkUp;
            }
        }

        return count;
    }

    int CalcCount (TRankNode* _pkNode)
    {
        if (_pkNode == nullptr) {
            return 0;
        }

        int count = _pkNode->m_nCount;

        while (_pkNode->m_pkNext != nullptr)
        {
            _pkNode = _pkNode->m_pkNext;

            if (_pkNode->m_pkUp != nullptr) {
                break;
            }

            count += _pkNode->m_nCount;
        }

        return count;
    }

    TRankNode* GetMapNode (TID _nID)
    {
        auto it = m_kNodeMap.find (_nID);
        if (it == m_kNodeMap.end ()) {
            return nullptr;
        }
        return it->second;
    }

    void SetMapNode (TRankNode* _pNode)
    {
        if (_pNode == nullptr) {
            return;
        }

        TID id = _pNode->m_nID;

        auto it = m_kNodeMap.find (id);
        if (it == m_kNodeMap.end ()) {
            m_kNodeMap.emplace (id, _pNode);
        }
        else {
            it->second = _pNode;
        }
    }

    void RemoveMapNode (TID _nID)
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
        TRankNode* node = nullptr;
        if (!m_kPool.empty ())
        {
            node = m_kPool.back ();

            node->m_nLevel = _nLevel;
            node->m_nCount = _nCount;
            node->m_nID = _nID;
            node->m_nScore = _nScore;

            m_kPool.pop_back ();
        }

        if (node == nullptr) {
            node = new TRankNode (_nLevel, _nCount, _nID, _nScore);
        }

        return node;
    }

    void PushNode (TRankNode*& _rpkNode)
    {
        if (_rpkNode == nullptr) {
            return;
        }

        _rpkNode->m_pkUp = nullptr;
        _rpkNode->m_pkDown = nullptr;
        _rpkNode->m_pkPrev = nullptr;
        _rpkNode->m_pkNext = nullptr;

        m_kPool.emplace_back (_rpkNode);

        _rpkNode = nullptr;
    }

    void ClearList ()
    {
        TRankNode* node = m_pkRoot;
        while (node != nullptr)
        {
            TRankNode* down = node->m_pkDown;
            while (node != nullptr)
            {
                TRankNode* next = node->m_pkNext;
                delete node;
                node = next;
            }
            node = down;
        }

        m_pkRoot = nullptr;
        m_kNodeMap.clear ();
    }

    void ClearPool ()
    {
        for (auto& node : m_kPool)
        {
            if (node != nullptr) {
                delete node;
            }
        }

        m_kPool.clear ();
        m_kPool.shrink_to_fit ();
    }

protected:
    TRankNode* m_pkRoot;
    std::unordered_map<TID, TRankNode*> m_kNodeMap;

private:
    std::vector<TRankNode*> m_kPool;
};
