#include <algorithm>
#include <iostream>
#include <list>

#include "reach.h"

int stack[10000];

void print_stack(size_t depth) {
    for (size_t i = 0; i < depth && i < 10000; ++i) {
        cout << stack[i] << "-";
    }
    if (depth < 10000) {
        cout << stack[depth] << endl;
    }
}

struct DFSData {
    DFSData(
        const size_t edgeIndex,
        const pair<double, double>& borders,
        const SPDI& spdi,
        const SPDIReachTask& reachTask,
        const size_t depth,
        const unordered_set<size_t>& visitedEdges = unordered_set<size_t>(),
        const unordered_set<string>& visitedCycles = unordered_set<string>(),
        const vector<size_t>& curResidualPath = vector<size_t>()
    )
        : EdgeIndex(edgeIndex)
        , Borders(borders)
        , Spdi(spdi)
        , ReachTask(reachTask)
        , Depth(depth)
    {
        VisitedEdges = unordered_set<size_t>();
        VisitedCycles = unordered_set<string>();
        CurResidualPath = vector<size_t>();

        for (const auto& edge : visitedEdges) {
            VisitedEdges.insert(edge);
        }

        for (const auto& cycle : visitedCycles) {
            VisitedCycles.insert(cycle);
        }

        for (const auto& edge : curResidualPath) {
            CurResidualPath.push_back(edge);
        }
    }

    size_t EdgeIndex;
    const pair<double, double> Borders;
    const SPDI& Spdi;
    const SPDIReachTask& ReachTask;
    unordered_set<size_t> VisitedEdges;
    unordered_set<string> VisitedCycles;
    vector<size_t> CurResidualPath;
    size_t Depth;
};

bool isFinState(const size_t edgeIndex, const pair<double, double>& borders, const SPDIReachTask& reachTask) {
    auto it = reachTask.FinalEdgeParts.find(edgeIndex);

    if (it == reachTask.FinalEdgeParts.end()) {
        return false;
    } else {
        return ImagesIntersect(it->second, borders);
    }
}

pair<bool, pair<double, double> > IterateCycleAndCheckFinalState(
    const vector<size_t>& cycle,
    const pair<double, double>& borders,
    const SPDI& spdi,
    const SPDIReachTask& reachTask
) {
    auto im = borders;

    for (size_t i = 0; i < cycle.size() - 1; ++i) {
        if (!ValidImage(im)) {
            return make_pair(false, make_pair(-1, -2));
        } else {

            if (isFinState(cycle[i], im, reachTask)) {
                return make_pair(true, im);
            }

            im = SuccInt(
                im.first,
                im.second,
                spdi.Edges[cycle[i]],
                spdi.Edges[cycle[i + 1]],
                spdi.EdgesConnections[cycle[i]].second.first,
                spdi.EdgesConnections[cycle[i]].second.second
            );
        }
    }

    if (ValidImage(im)) {
        return make_pair(isFinState(cycle.back(), im, reachTask), im);
    } else {
        return make_pair(false, make_pair(-1, -2));
    }
}

pair<bool, vector<pair<double, double> > > TestCycleAndGetFinalImages(
    const vector<size_t>& cycle,
    const pair<double, double>& borders,
    const SPDI& spdi,
    const SPDIReachTask& reachTask
) {
    const auto res = IterateCycleAndCheckFinalState(cycle, borders, spdi, reachTask);
    if (res.first) {
        return make_pair(true, vector<pair<double, double> >(1, res.second));
    }

    auto imPrev = borders;
    auto imSucc = res.second;
    auto answer = make_pair(false, vector<pair<double, double> >(1, imPrev));

    while (!ImagesIntersect(imPrev, imSucc)) {
        /*
            DIE cycle
        */
        while ((!ImagesIntersect(imPrev, imSucc)) && ValidImage(imSucc)) {
            imPrev = imSucc;
            const auto res = IterateCycleAndCheckFinalState(cycle, imSucc, spdi, reachTask);

            answer.first |= res.first;
            answer.second.push_back(res.second);

            imSucc = res.second;

            if (res.first) {
                return answer;
            }
        }

        if (!ValidImage(imSucc)) {
            return answer;
        }
    }

    /*
        STAY, EXIT-LEFT, EXIT-RIGHT, EXIT-BOTH cycles
    */
    double left1 = SuccPointAMFSigma(imPrev.first, cycle, 'L', spdi);
    double left2 = SuccPointAMFSigma(imPrev.second, cycle, 'L', spdi);
    double leftA = (left2 - left1) / (imPrev.second - imPrev.first);
    double leftB = left1 - leftA * imPrev.first;
    double lim1L = CalcLimitPoint(leftA, leftB, imPrev.first);
    double lim1R = CalcLimitPoint(leftA, leftB, imPrev.second);

    double right1 = SuccPointAMFSigma(imPrev.first, cycle, 'R', spdi);
    double right2 = SuccPointAMFSigma(imPrev.second, cycle, 'R', spdi);
    double rightA = (right2 - right1) / (imPrev.second - imPrev.first);
    double rightB = right1 - rightA * imPrev.first;
    double lim2L = CalcLimitPoint(rightA, rightB, imPrev.first);
    double lim2R = CalcLimitPoint(rightA, rightB, imPrev.second);

    double leftLim = min(min(lim1L, lim1R), min(lim2L, lim2R));
    double rightLim = max(max(lim1L, lim1R), max(lim2L, lim2R));

    double lFin = max(0.0, min(min(imPrev.first, leftLim), min(imPrev.second, rightLim)));
    double rFin = min(1.0, max(max(imPrev.first, leftLim), max(imPrev.second, rightLim)));
    pair<double, double> finalDomain = make_pair(lFin, rFin);

    if (!ValidImage(finalDomain)) {
        return make_pair(false, vector<pair<double, double> >(1, make_pair(-1, -2)));
    }

    const auto ans = IterateCycleAndCheckFinalState(cycle, finalDomain, spdi, reachTask);
    return make_pair(ans.first, vector<pair<double, double> >(1, finalDomain));
}

void* DFSSignaturesExploration(void* threadArguments) {
    DFSData* threadData = (DFSData*) threadArguments;

    const size_t edgeIndex = threadData->EdgeIndex;
    const pair<double, double>& borders = threadData->Borders;
    const SPDI& spdi = threadData->Spdi;
    const SPDIReachTask& reachTask = threadData->ReachTask;
    unordered_set<size_t>& visitedEdges = threadData->VisitedEdges;
    unordered_set<string>& visitedCycles = threadData->VisitedCycles;
    vector<size_t>& curResidualPath = threadData->CurResidualPath;
    const size_t depth = threadData->Depth;

    if (Debug) {
        for (size_t i = 0; i < depth; ++i) {
            cout << " ";
        }
        cout << spdi.EdgeIdMap[edgeIndex] << " [" << borders.first << ", " << borders.second << "]" << endl;
    }

    stack[depth] = edgeIndex;

    if (Answer) {
        goto exit;
    }

    if (isFinState(edgeIndex, borders, reachTask)) {
        pthread_mutex_lock(&AnswerMutex);
        Answer = true;
        pthread_mutex_unlock(&AnswerMutex);

        if (Debug) {
            print_stack(depth);
        }
        goto exit;
    }

    if (visitedEdges.find(edgeIndex) != visitedEdges.end()) {
        vector<size_t> cycle; //store all edges of cycle in reversed order
        bool inCycle = true;

        for (int i = curResidualPath.size() - 1; i >= 0; --i) {
            if (inCycle) {
                cycle.push_back(curResidualPath[i]);
                visitedEdges.erase(curResidualPath[i]);
            }

            if (curResidualPath[i] == edgeIndex) {
                inCycle = false;
            }
        }

        if (inCycle) {
            /*
                cycle ends not in current residual path, forbidden
            */
            goto exit;
        }

        vector<size_t> sortedCycle(cycle);
        sort(sortedCycle.begin(), sortedCycle.end());

        string cycleId = "";
        for (size_t i = 0; i < sortedCycle.size(); ++i) {
            cycleId += to_string(sortedCycle[i]) + "_";
        }

        if (visitedCycles.find(cycleId) != visitedCycles.end()) {
            goto exit;
        } else {
            visitedCycles.insert(cycleId);
        }

        reverse(cycle.begin(), cycle.end());    //restore original order of cycle edges
        cycle.push_back(edgeIndex);             //close cycle with the initial edge

        const auto res = TestCycleAndGetFinalImages(cycle, borders, spdi, reachTask);

        if (res.first) {
            pthread_mutex_lock(&AnswerMutex);
            Answer = true;
            pthread_mutex_unlock(&AnswerMutex);

            if (Debug) {
                print_stack(depth);
            }
            goto exit;
        } else {
            for (const auto& im : res.second) {
                if (!ValidImage(im)) {
                    goto exit;
                }

                vec2 c1 = spdi.EdgesConnections[edgeIndex].second.first;
                vec2 c2 = spdi.EdgesConnections[edgeIndex].second.second;
                size_t nextEdgeIndex = 0, nextEdgesSize = spdi.EdgesConnections[edgeIndex].first.size();
                vector<pthread_t> threadIds;

                for (const auto& nextEdge : spdi.EdgesConnections[edgeIndex].first) {
                    pair<double, double> image = SuccInt(im.first, im.second, spdi.Edges[edgeIndex], spdi.Edges[nextEdge], c1, c2);

                    if (ValidImage(image)) {
                        pthread_mutex_lock(&FreeThreadsMutex);

                        if ((FreeThreads > 0) && (++nextEdgeIndex != nextEdgesSize)) {
                            --FreeThreads;
                            threadIds.push_back(0);
                            pthread_create(&threadIds[threadIds.size() - 1], &ThreadAttributes, DFSSignaturesExploration,
                                    new DFSData(nextEdge, image, spdi, reachTask, depth + 1, visitedEdges, visitedCycles));
                            pthread_mutex_unlock(&FreeThreadsMutex);
                        } else {
                            pthread_mutex_unlock(&FreeThreadsMutex);

                            DFSSignaturesExploration(new DFSData(nextEdge, image, spdi, reachTask, depth + 1, visitedEdges, visitedCycles));
                        }
                    }
                }

                for (const auto& threadId : threadIds) {
                    pthread_join(threadId, NULL);
                    pthread_mutex_lock(&FreeThreadsMutex);
                    ++FreeThreads;
                    pthread_mutex_unlock(&FreeThreadsMutex);
                }
            }
        }

    } else {
        visitedEdges.insert(edgeIndex);
        curResidualPath.push_back(edgeIndex);

        vec2 c1 = spdi.EdgesConnections[edgeIndex].second.first;
        vec2 c2 = spdi.EdgesConnections[edgeIndex].second.second;
        size_t nextEdgeIndex = 0, nextEdgesSize = spdi.EdgesConnections[edgeIndex].first.size();
        vector<pthread_t> threadIds;

        for (const auto& nextEdge : spdi.EdgesConnections[edgeIndex].first) {
            pair<double, double> image = SuccInt(borders.first, borders.second, spdi.Edges[edgeIndex], spdi.Edges[nextEdge], c1, c2);

            if (ValidImage(image)) {
                pthread_mutex_lock(&FreeThreadsMutex);

                if ((FreeThreads > 0) && (++nextEdgeIndex != nextEdgesSize)) {
                    --FreeThreads;
                    threadIds.push_back(0);
                    pthread_create(&threadIds[threadIds.size() - 1], &ThreadAttributes, DFSSignaturesExploration,
                            new DFSData(nextEdge, image, spdi, reachTask, depth + 1, visitedEdges, visitedCycles, curResidualPath));
                    pthread_mutex_unlock(&FreeThreadsMutex);
                } else {
                    pthread_mutex_unlock(&FreeThreadsMutex);

                    DFSSignaturesExploration(new DFSData(nextEdge, image, spdi, reachTask, depth + 1, visitedEdges, visitedCycles, curResidualPath));
                }
            }
        }

        for (const auto& threadId : threadIds) {
            pthread_join(threadId, NULL);
            pthread_mutex_lock(&FreeThreadsMutex);
            ++FreeThreads;
            pthread_mutex_unlock(&FreeThreadsMutex);
        }
    }

exit:
    delete threadData;
    return NULL;
}

void SolveReachTask(const SPDI& spdi, const SPDIReachTask& reachTask) {
    size_t curStartEdgeIndex = 0, edgePartsSize = reachTask.StartEdgeParts.size();
    vector<pthread_t> threadIds;
    for (const auto& startEdge : reachTask.StartEdgeParts) {
        pthread_mutex_lock(&FreeThreadsMutex);

        if ((FreeThreads > 0) && (++curStartEdgeIndex != edgePartsSize)) {
            --FreeThreads;
            threadIds.push_back(0);
            pthread_create(&threadIds[threadIds.size() - 1], &ThreadAttributes, DFSSignaturesExploration,
                    new DFSData(startEdge.first, startEdge.second, spdi, reachTask, 0));
            pthread_mutex_unlock(&FreeThreadsMutex);
        } else {
            pthread_mutex_unlock(&FreeThreadsMutex);

            DFSSignaturesExploration(new DFSData(startEdge.first, startEdge.second, spdi, reachTask, 0));
        }
    }

    for (const auto& threadId : threadIds) {
        pthread_join(threadId, NULL);
        pthread_mutex_lock(&FreeThreadsMutex);
        ++FreeThreads;
        pthread_mutex_unlock(&FreeThreadsMutex);
    }

    cout << (Answer ? 1 : 0) << endl;
}
