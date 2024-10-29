#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <chrono>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <cassert>

using namespace std;

class Metrics
{
public:
    unordered_map<string, std::chrono::nanoseconds> metrics;

    void Add(string name, std::chrono::nanoseconds value)
    {
        if (metrics.find(name) == metrics.end())
        {
            metrics[name] = value;
        }
        else
        {
            metrics[name] += value;
        }
    }

    void Print()
    {
        // std::chrono::nanoseconds total_time = metrics.at("total_time");
        auto total_time = chrono::duration_cast<chrono::milliseconds>(metrics.at("total_time")).count();

        vector<pair<string, std::chrono::nanoseconds>> ordered_metrics(metrics.begin(), metrics.end());
        sort(ordered_metrics.begin(), ordered_metrics.end(), [](const pair<string, std::chrono::nanoseconds> &a, const pair<string, std::chrono::nanoseconds> &b)
             { return a.first < b.first; });

        // for (auto it = metrics.begin(); it != metrics.end(); it++) {
        for (auto &entry : ordered_metrics)
        {
            auto name = entry.first;
            auto val = entry.second;
            auto ms = chrono::duration_cast<chrono::milliseconds>(val).count();
            printf("%s: %lld msec (%.2f%%)\n",
                   name.c_str(),
                   ms,
                   100.0f * ms / total_time);
        }
    }

    void RecordSpriteUpdatePhyscis(std::chrono::nanoseconds delta)
    {
        Add("sprite_update_physics_usec", delta);
    }
    void RecordSpriteQuadTreeUpdate(std::chrono::nanoseconds delta)
    {
        Add("sprite_quad_tree_update_usec", delta);
    }
    void RecordSpriteCollide(std::chrono::nanoseconds delta)
    {
        Add("sprite_collide_usec", delta);
    }
    void RecordQuadCollision(std::chrono::nanoseconds delta)
    {
        Add("quad_collision_usec", delta);
    }
    void RecordQuadTreeInsert(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_insert", delta);
    }
    void RecordQuadTreeRemove(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_remove", delta);
    }
    void RecordQuadClean(std::chrono::nanoseconds delta)
    {
        Add("quad_clean", delta);
    }
    void RecordSceneDraw(std::chrono::nanoseconds delta)
    {
        Add("scene_draw", delta);
    }
    void RecordTotalTime(std::chrono::nanoseconds delta)
    {
        Add("total_time", delta);
    }
    void RecordQuadTreeRemoveInner(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_remove_inner", delta);
    }
    void RecordQuadFindLeaves(std::chrono::nanoseconds delta)
    {
        Add("quad_find_leaves", delta);
    }
    void RecordQuadFindLeavesList(std::chrono::nanoseconds delta)
    {
        Add("quad_find_leaves_list", delta);
    }
    void RecordQuadTreeInsertNode(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_insert_node", delta);
    }
    void RecordQuadInsertLeafNode(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_insert_leaf_node", delta);
    }
};

extern Metrics METRICS;
