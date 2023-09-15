#include <chrono>

#include "RankList.h"

template<int N, int Size = 100000, int Times = 10>
void Test ()
{
    using TRankList = CRankList<int, int, N>;

    long long insert = 0;
    long long update = 0;
    long long check = 0;
    long long remove = 0;
    int maxLevel = 0;
    TRankList rankList;

    for (int times = Times; times > 0; times--)
    {
        rankList.Clear ();

        {
            auto start = std::chrono::steady_clock::now ();

            for (int i = 1; i <= Size; i++) {
                rankList.SetRank (i, rand ());
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            insert += ms.count ();
        }

        if (rankList.GetMaxLevel () > maxLevel) {
            maxLevel = rankList.GetMaxLevel ();
        }

        {
            auto start = std::chrono::steady_clock::now ();

            for (int i = 1; i <= Size; i++) {
                if (rand () % 2 == 0) {
                    rankList.SetRank ((rand () % Size) + 1, rand ());
                }
                else {
                    rankList.RemoveRank ((rand () % Size) + 1);
                }
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            update += ms.count ();
        }

        {
            auto start = std::chrono::steady_clock::now ();

            rankList.CheckScore ();
            rankList.CheckRank ();

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            check += ms.count ();
        }

        {
            auto start = std::chrono::steady_clock::now ();

            for (int i = 1; i <= Size; i++) {
                rankList.RemoveRank (i);
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start);
            remove += ms.count ();
        }

        rankList.Print ();
    }

    std::cout << "N: " << N << ", Size: " << Size << ", MaxLevel: " << maxLevel << std::endl;
    std::cout << "Insert: " << insert / 1000.0 / Times << "s, ";
    std::cout << "Update: " << update / 1000.0 / Times << "s, ";
    std::cout << "Check: " << check / 1000.0 / Times << "s, ";
    std::cout << "Remove: " << remove / 1000.0 / Times << "s" << std::endl;
}

int main ()
{
    srand (static_cast<unsigned int> (time (nullptr)));

    Test<2> ();
    //Test<3> ();
    Test<4> ();
    //Test<5> ();
    //Test<6> ();
    //Test<7> ();
    Test<8> ();
    //Test<10> ();
    //Test<12> ();
    //Test<14> ();

    return 0;
}