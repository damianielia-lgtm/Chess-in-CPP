#pragma once

#include "../engine/search.h"

struct NullObserver {
    void on_node() noexcept {}
    void on_leaf() noexcept {}
};

struct BaisicStatsObserver {
    SearchStats& stats;

    void on_node() noexcept { stats.nodes++; }
    void on_leaf() noexcept { stats.leaf_nodes++; }
};
