#pragma once

#include <iostream>
#include <chrono>

#include <cmath>
#include <forward_list>
#include <stack>
#include <unordered_map>
#include <vector>

template<typename TRankID>
class CRankData
{
public:
    using TID = TRankID;

    CRankData ()
        : m_nID (0)
        , m_nScore (0)
    {
    }

    CRankData (TID _nID, int _nScore)
        : m_nID (_nID)
        , m_nScore (_nScore)
    {
    }

    virtual ~CRankData ()
    {
    }

    inline void SetID (TID _nID) { m_nID = _nID; }
    inline void SetScore (int _nScore) { m_nScore = _nScore; }
    inline TID GetID () const { return m_nID; }
    inline int GetScore () const { return m_nScore; }

protected:
    TID m_nID;
    int m_nScore;
};

template<typename TRankData>
class CRankNode
{
public:
    using TID = typename TRankData::TID;

public:
    CRankNode (int _nLevel, int _nCount, TRankData& _kRankData)
        : m_nLevel (_nLevel)
        , m_nCount (_nCount)
        , m_kRankData (_kRankData)
        , m_pUp (nullptr)
        , m_pDown (nullptr)
        , m_pPrev (nullptr)
        , m_pNext (nullptr)
    {
    }

    virtual ~CRankNode ()
    {
    }

    inline void SetData (TRankData& _kRankData) { m_kRankData = _kRankData; }
    inline void SetID (TID _nID) { m_kRankData.SetID (_nID); }
    inline void SetScore (int _nScore) { m_kRankData.SetScore (_nScore); }
    inline TID GetID () const { return m_kRankData.GetID (); }
    inline int GetScore () const { return m_kRankData.GetScore (); }

    int m_nLevel;
    int m_nCount;
    TRankData m_kRankData;

    CRankNode* m_pUp;
    CRankNode* m_pDown;
    CRankNode* m_pPrev;
    CRankNode* m_pNext;
};

template<typename TRankData>
class CRankList
{
public:
    using TID = typename TRankData::TID;
    using TRankNode = CRankNode<TRankData>;

public:
    CRankList ()
        : m_pRoot (nullptr)
    {
    }

    virtual ~CRankList ()
    {
        TRankNode* pHead = m_pRoot;
        while (pHead != nullptr)
        {
            TRankNode* pNode = pHead;
            pHead = pHead->m_pDown;
            while (pNode != nullptr)
            {
                TRankNode* pTemp = pNode;
                pNode = pNode->m_pNext;
                delete pTemp;
            }
        }
    }

    int GetRank (TID _nID)
    {
        TRankNode* pRankNode = GetRankNode (_nID);
        if (pRankNode == nullptr) {
            return 0;
        }

        return CalcRank (pRankNode) + 1;
    }

    /*
    4| 9(10)
    3| 9(4)                5(6)
    2| 9(1) 8(3)           5(3)           3(3)
    1| 9(1) 8(1) 7(1) 6(1) 5(1) 4(1) 4(1) 3(1) 2(1) 1(1)

    */                               //7

    void SetRank (TRankData& _kRankData)
    {
        TID nID = _kRankData.GetID ();
        int nScore = _kRankData.GetScore ();

        TRankNode* pRankNode = GetRankNode (nID);
        if (pRankNode != nullptr && pRankNode->GetScore () == nScore) {
            return;
        }

        RemoveRank (nID);

        if (m_pRoot == nullptr)
        {
            TRankNode* pRoot = InsertRoot (_kRankData);
            if (pRoot == nullptr) {
                return;
            }
            return;
        }

        if (nScore > m_pRoot->GetScore ())
        {
            // 交換第一位
            TRankNode* pCurrent = m_pRoot;
            while (pCurrent->m_pDown != nullptr)
            {
                pCurrent->m_nCount++;
                pCurrent->m_kRankData = _kRankData;
                pCurrent = pCurrent->m_pDown;
            }

            TRankNode* pNewNode = InsertNext (pCurrent, pCurrent->m_kRankData);
            if (pNewNode == nullptr) {
                return;
            }

            pCurrent->m_kRankData = _kRankData;
            SetRankNode (pCurrent);
        }
        else
        {
            std::stack<TRankNode*> kParents;
            TRankNode* pNode = FindNode (nScore, m_pRoot, kParents);
            if (pNode == nullptr) {
                return;
            }

            TRankNode* pNewNode = InsertNext (pNode, _kRankData);
            if (pNewNode == nullptr) {
                return;
            }

            while (!kParents.empty ())
            {
                TRankNode* pParent = kParents.top ();

                pParent->m_nCount++;
                if (pParent->m_nCount >= pow (3, pParent->m_nLevel)) {
                    InsertUp (pParent, GetTopNode (pNewNode));
                }

                kParents.pop ();
            }
        }

        // 擴展層級
        if (m_pRoot->m_nCount >= pow (3, m_pRoot->m_nLevel))
        {
            TRankNode* pRoot = IncreaseLevel ();
            if (pRoot == nullptr) {
                return;
            }
        }
    }

    void RemoveRank (TID _nID)
    {

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
            //std::cout << "level: " << pDown->m_nLevel << std::endl;
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
        while (pNode != nullptr) {
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
    TRankNode* FindNode (int _nScore, TRankNode* _pCurrent, std::stack<TRankNode*>& _kParents)
    {
        if (_pCurrent == nullptr) {
            return nullptr;
        }

        TRankNode* pCurrent = _pCurrent;
        TRankNode* pNext = pCurrent->m_pNext;
        while (pNext != nullptr)
        {
            if (_nScore > pNext->GetScore ()) {
                break;
            }
            pCurrent = pNext;
            pNext = pNext->m_pNext;
        }

        if (pCurrent->m_pDown != nullptr) {
            _kParents.push (pCurrent);
            return FindNode (_nScore, pCurrent->m_pDown, _kParents);
        }

        return pCurrent;
    }

    TRankNode* GetTopNode (TRankNode* _pCurrent)
    {
        if (_pCurrent == nullptr) {
            return nullptr;
        }

        while (_pCurrent->m_pUp != nullptr) {
            _pCurrent = _pCurrent->m_pUp;
        }

        return _pCurrent;
    }

    TRankNode* InsertRoot (TRankData& _kRankData)
    {
        if (m_pRoot != nullptr) {
            return m_pRoot;
        }

        m_pRoot = new TRankNode (2, 1, _kRankData);

        TRankNode* pNewNode = new TRankNode (1, 1, _kRankData);
        pNewNode->m_pUp = m_pRoot;

        m_pRoot->m_pDown = pNewNode;

        SetRankNode (m_pRoot);

        return m_pRoot;
    }

    TRankNode* InsertNext (TRankNode* _pNode, TRankData& _kRankData)
    {
        if (_pNode == nullptr || _pNode->m_nLevel != 1) {
            return nullptr;
        }

        TRankNode* pNewNode = new TRankNode (1, 1, _kRankData);
        pNewNode->m_pNext = _pNode->m_pNext;
        pNewNode->m_pPrev = _pNode;

        if (_pNode->m_pNext != nullptr) {
            _pNode->m_pNext->m_pPrev = pNewNode;
        }
        _pNode->m_pNext = pNewNode;

        SetRankNode (pNewNode);

        return pNewNode;
    }

    TRankNode* InsertUp (TRankNode* _pParent, TRankNode* _pNode)
    {
        if (_pParent == nullptr || _pNode == nullptr || _pParent->m_nLevel != _pNode->m_nLevel + 1) {
            return nullptr;
        }

        TRankNode* pNewNode = new TRankNode (_pParent->m_nLevel, GetNextCount (_pNode), _pNode->m_kRankData);
        pNewNode->m_pDown = _pNode;
        pNewNode->m_pNext = _pParent->m_pNext;
        pNewNode->m_pPrev = _pParent;

        if (_pParent->m_pNext != nullptr) {
            _pParent->m_pNext->m_pPrev = pNewNode;
        }
        _pParent->m_pNext = pNewNode;
        _pParent->m_nCount -= pNewNode->m_nCount;

        _pNode->m_pUp = pNewNode;

        SetRankNode (pNewNode);

        return pNewNode;
    }

    TRankNode* IncreaseLevel ()
    {
        if (m_pRoot == nullptr) {
            return nullptr;
        }

        TRankNode* pNewNode = new TRankNode (m_pRoot->m_nLevel + 1, GetNextCount (m_pRoot), m_pRoot->m_kRankData);
        pNewNode->m_pDown = m_pRoot;

        m_pRoot->m_pUp = pNewNode;
        m_pRoot = pNewNode;

        SetRankNode (m_pRoot);

        return m_pRoot;
    }

    int CalcRank (TRankNode* _pCurrent)
    {
        if (_pCurrent == nullptr) {
            return 0;
        }

        int nCount = 0;
        TRankNode* pPrev = _pCurrent->m_pPrev;
        while (pPrev != nullptr)
        {
            nCount += pPrev->m_nCount;

            if (pPrev->m_pUp != nullptr) {
                break;
            }
            pPrev = pPrev->m_pPrev;
        }

        if (pPrev != nullptr) {
            return nCount + CalcRank (pPrev->m_pUp);
        }

        return nCount;
    }

    int GetNextCount (TRankNode* _pCurrent)
    {
        if (_pCurrent == nullptr) {
            return 0;
        }

        int nCount = _pCurrent->m_nCount;
        TRankNode* pNext = _pCurrent->m_pNext;
        while (pNext != nullptr)
        {
            if (pNext->m_pUp != nullptr) {
                break;
            }
            nCount += pNext->m_nCount;
            pNext = pNext->m_pNext;
        }

        return nCount;
    }

    TRankNode* GetRankNode (TID _nID)
    {
        auto it = m_kNodeMap.find (_nID);
        if (it == m_kNodeMap.end ()) {
            return nullptr;
        }
        return it->second;
    }

    void SetRankNode (TRankNode* _pNode)
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

    void RemoveRankNode (TID _nID)
    {
        m_kNodeMap.erase (_nID);
    }

    TRankNode* m_pRoot;
    std::unordered_map<TID, TRankNode*> m_kNodeMap;
};

class CMyRankData : public CRankData<int>
{
};

int main ()
{

    CRankList<CMyRankData> kRankList;
    CMyRankData kMyRankData;
    for (int i = 1; i <= 100000; i++)
    {
        int id = i;
        int score = rand ();
        kMyRankData.SetID (i);
        kMyRankData.SetScore (score);
        kRankList.SetRank (kMyRankData);
    }    
    kRankList.Print ();


    //for (int i = 100001; i <= 100200; i++)
    //{
    //    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now ();
    //    int id = i;
    //    int score = rand ();
    //    kMyRankData.SetID (i);
    //    kMyRankData.SetScore (score);
    //    kRankList.SetRank (kMyRankData);
    //    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now ();

    //    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count () << "[ms]" << std::endl;
    //    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count () << "[ns]" << std::endl;
    //}


    return 0;
}