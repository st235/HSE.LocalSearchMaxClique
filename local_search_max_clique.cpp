#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include <iterator>
#include <cstddef>
#include <random>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

using namespace std;

namespace std {

template<class T>
class linked_unordered_set {
private:
    template<class V>
    struct LinkedNode {
        V item;
        LinkedNode<V>* prev;
        LinkedNode<V>* next;

        explicit LinkedNode(const V& item):
                item(item),
                prev(nullptr),
                next(nullptr) {
            // empty on purpose
        }
        LinkedNode(const LinkedNode<V>& node) = default;
        LinkedNode<V>& operator=(const LinkedNode<V>& node) = default;

        ~LinkedNode() = default;
    };

    size_t size_;
    size_t capacity_;

    std::unordered_map<T, LinkedNode<T>*> lookup_;
    LinkedNode<T>* head_;
    LinkedNode<T>* tail_;

    [[nodiscard]] LinkedNode<T>* addToList(const T& item) {
        auto* node = new LinkedNode<T>(item);

        if (head_ == nullptr) {
            assert(tail_ == nullptr);

            head_ = node;
            tail_ = node;
        } else {
            // head_ is not null.
            assert(tail_ != nullptr);

            tail_->next = node;
            node->prev = tail_;
            tail_ = node;
        }

        return node;
    }

    void removeFromList(const T& item) {
        LinkedNode<T>* node = lookup_.at(item);

        LinkedNode<T>* prev = node->prev;
        LinkedNode<T>* next = node->next;

        if (prev != nullptr) {
            prev->next = next;
        }

        if (next != nullptr) {
            next->prev = prev;
        }

        if (head_ == node) {
            head_ = next;
        }

        if (tail_ == node) {
            tail_ = prev;
        }

        delete node;
    }

    LinkedNode<T>* createDeepCopy(LinkedNode<T>* list) {
        if (list == nullptr) {
            return nullptr;
        }

        auto* node = new LinkedNode<T>(list->item);

        auto* next = createDeepCopy(node->next);
        if (next != nullptr) {
            next->prev = node;
        }

        node->next = next;
        return node;
    }

public:
    explicit linked_unordered_set(size_t capacity):
            size_(0),
            capacity_(capacity),
            lookup_(),
            head_(nullptr),
            tail_(nullptr) {
        // empty on purpose
    }

    linked_unordered_set(const linked_unordered_set<T>& that):
            size_(that.size_),
            capacity_(that.capacity_),
            lookup_(),
            head_(nullptr),
            tail_(nullptr) {
        head_ = createDeepCopy(that.head_);
        tail_ = head_;

        while (tail_ != nullptr && tail_->next != nullptr) {
            lookup_.insert({ tail_->item, tail_ });
            tail_ = tail_->next;
        }

        if (tail_ != nullptr) {
            lookup_.insert({ tail_->item, tail_ });
        }
    }

    linked_unordered_set<T>& operator=(const linked_unordered_set<T>& that) {
        if (this != &that) {
            size_ = that.size_;
            capacity_ = that.capacity_;

            lookup_.clear();
            LinkedNode<T>* node = head_;

            while (node != nullptr) {
                auto* next = node->next;
                delete node;
                node = next;
            }

            head_ = createDeepCopy(that.head_);
            tail_ = head_;

            while (tail_ != nullptr && tail_->next != nullptr) {
                lookup_.insert({ tail_->item, tail_ });
                tail_ = tail_->next;
            }

            if (tail_ != nullptr) {
                lookup_.insert({ tail_->item, tail_ });
            }
        }

        return &this;
    }

    void insert(const T& item) {
        if (contains(item)) {
            remove(item);
        }

        auto* node = addToList(item);
        lookup_.insert({ item, node });

        size_ += 1;

        if (size_ > capacity_) {
            remove();
        }
    }

    bool remove(const T& item) {
        if (!contains(item)) {
            return false;
        }

        removeFromList(item);
        lookup_.erase(item);
        size_ -= 1;
        return true;
    }

    T remove() {
        if (empty()) {
            throw std::runtime_error("Cannot remove item from empty set.");
        }

        assert(head_ != nullptr && tail_ != nullptr);
        assert(!lookup_.empty());

        // We need to explicitly copy
        // the item before it would be removed.
        T item(head_->item);
        remove(item);
        return item;
    }

    [[nodiscard]] inline bool contains(const T& item) const {
        return lookup_.find(item) != lookup_.end();
    }

    [[nodiscard]] inline bool empty() const {
        bool is_empty = lookup_.empty();
        if (is_empty) {
            assert(head_ == nullptr && tail_ == nullptr);
        } else {
            assert(head_ != nullptr && tail_ != nullptr);
        }
        return is_empty;
    }

    [[nodiscard]] inline size_t size() const {
        return size_;
    }

    ~linked_unordered_set() {
        LinkedNode<T>* node = head_;

        while (node != nullptr) {
            LinkedNode<T>* real_next = node->next;
            delete node;
            node = real_next;
        }
    }
};

} // namespace std

namespace {

double RoundTo(double value, double precision = 1.0) {
    return std::round(value / precision) * precision;
}

int32_t GenerateInRange(int32_t start, int32_t finish) {
    int32_t width = finish - start + 1;
    return static_cast<int32_t>(std::rand() % width + start);
}

class TabooList {
private:
    std::linked_unordered_set<int32_t> added_vertices_;
    std::linked_unordered_set<int32_t> removed_vertices_;

public:
    TabooList(size_t added_tabu_size,
              size_t removed_tabu_size):
             added_vertices_(added_tabu_size),
             removed_vertices_(removed_tabu_size) {
        assert(added_tabu_size > 0);
        assert(removed_tabu_size > 0);
    }

    TabooList(const TabooList& that) = default;
    TabooList& operator=(const TabooList& that) = default;

    void RestrictRemovedVertex(int32_t vertex) {
        removed_vertices_.insert(vertex);
    }

    void RestrictAddedVertex(int32_t vertex) {
        added_vertices_.insert(vertex);
    }

    [[nodiscard]] inline bool IsInRemovedList(int32_t vertex) const {
        return removed_vertices_.contains(vertex);
    }

    [[nodiscard]] inline bool IsInAddedList(int32_t vertex) const {
        return added_vertices_.contains(vertex);
    }

    ~TabooList() = default;
};

class Clique {
private:
    size_t size_;

    int32_t index_q_;
    int32_t index_c_;

    std::vector<std::unordered_set<int32_t>> vertices_neighbours_;
    std::vector<std::unordered_set<int32_t>> vertices_non_neighbours_;

    std::vector<int32_t> qco_;
    std::vector<int32_t> index_;
    std::vector<int32_t> tightness_;

    TabooList tabu_list_;

    [[nodiscard]] inline bool IsClique(int vertex) const {
        const auto& vertex_index = index_[vertex];
        assert(vertex_index >= 0 && vertex_index < size_);
        return vertex_index <= index_q_;
    }

    [[nodiscard]] inline bool IsCandidate(int vertex) const {
        const auto& vertex_index = index_[vertex];
        assert(vertex_index >= 0 && vertex_index < size_);
        return vertex_index > index_q_ && vertex_index <= index_c_;
    }

    [[nodiscard]] inline bool HasCandidates() const {
        assert(index_c_ >= index_q_);
        return index_c_ >= 0 && index_c_ > index_q_;
    }

    [[nodiscard]] inline bool AreNeighbours(int32_t a, int32_t b) const {
        assert(a >= 0 && a < size_);
        assert(b >= 0 && b < size_);

        bool a_has_b = vertices_neighbours_[a].find(b) != vertices_neighbours_[a].end();
        bool b_has_a = vertices_neighbours_[b].find(a) != vertices_neighbours_[b].end();
        return a_has_b && b_has_a;
    }

    inline void SwapVerticesByQcoIndices(int32_t index_a, int32_t index_b) {
        assert(index_a >= 0 && index_a < size_);
        assert(index_b >= 0 && index_b < size_);

        // Vertex is index index_.
        // Index is index in qco_.
        const auto& vertex_a = qco_[index_a];
        const auto& vertex_b = qco_[index_b];

        std::swap(qco_[index_a], qco_[index_b]);
        std::swap(index_[vertex_a], index_[vertex_b]);
    }

public:
    Clique(size_t size,
           const std::vector<std::unordered_set<int32_t>>& graph):
            size_(size),
            index_q_(-1),
            index_c_(-1),
            vertices_neighbours_(graph),
            vertices_non_neighbours_(size),
            qco_(size),
            index_(size),
            tightness_(size),
            tabu_list_(3, 1) {
        assert(size >= 0);

        // All items are candidates as the clique is empty.
        index_c_ = static_cast<int32_t>(size) - 1;

        for (int i = 0; i < size; ++i) {
            const auto& adjacent_vertices = graph[i];

            for (int j = 0; j < size; ++j) {
                if (i == j) {
                    continue;
                }

                // Look that j is not in adjacency list of i.
                if (adjacent_vertices.find(j) == adjacent_vertices.end()) {
                    vertices_non_neighbours_[i].insert(j);
                }
            }
        }

        for (int32_t i = 0; i < size; i++) {
            qco_[i] = i;
            index_[i] = i;
            tightness_[i] = 0;
        }
    }

    Clique(const Clique& that) = default;
    Clique& operator=(const Clique& that) = default;

    void AddToClique(int32_t vertex) {
        // We should add only candidates to the clique.
        assert(IsCandidate(vertex));

        const auto& index_vertex = index_[vertex];

        // Now points to a candidate vertex.
        index_q_ += 1;

        SwapVerticesByQcoIndices(index_vertex, index_q_);

        for (const auto& non_neighbour: vertices_non_neighbours_[vertex]) {
            if (tightness_[non_neighbour] == 0) {
                RemoveFromCandidates(non_neighbour);
            }

            tightness_[non_neighbour] += 1;
        }
    }

    void RemoveFromClique(int32_t vertex) {
        assert(IsClique(vertex));

        const auto& index_vertex = index_[vertex];

        SwapVerticesByQcoIndices(index_vertex, index_q_);

        // We can decrease q after we swapped vertices.
        index_q_ -= 1;

        for (const auto& non_neighbour: vertices_non_neighbours_[vertex]) {
            tightness_[non_neighbour] -= 1;

            if (tightness_[non_neighbour] == 0) {
                AddToCandidates(non_neighbour);
            }
        }
    }

    void AddToCandidates(int32_t vertex) {
        assert(!IsCandidate(vertex));

        const auto& index_vertex = index_[vertex];

        index_c_ += 1;

        SwapVerticesByQcoIndices(index_vertex, index_c_);
    }

    void RemoveFromCandidates(int32_t vertex) {
        assert(IsCandidate(vertex));

        const auto& index_vertex = index_[vertex];
        SwapVerticesByQcoIndices(index_vertex, index_c_);

        index_c_ -= 1;
    }

    bool Swap1to2() {
        for (int32_t index_clique = 0; index_clique <= index_q_; index_clique++) {
            int vertex_clique = qco_[index_clique];

            // We should not remove recently added vertex.
            if (tabu_list_.IsInAddedList(vertex_clique)) {
                continue;
            }

            const auto& non_neighbours = vertices_non_neighbours_[vertex_clique];

            for (const auto& non_neighbour_a: non_neighbours) {
                if (tabu_list_.IsInRemovedList(non_neighbour_a)
                    || tightness_[non_neighbour_a] != 1) {
                    continue;
                }

                for (const auto& non_neighbour_b: non_neighbours) {
                    if (non_neighbour_a == non_neighbour_b) {
                        continue;
                    }

                    if (tabu_list_.IsInRemovedList(non_neighbour_b)
                        || tightness_[non_neighbour_b] != 1) {
                        continue;
                    }

                    if (!AreNeighbours(non_neighbour_a, non_neighbour_b)) {
                        continue;
                    }

                    RemoveFromClique(vertex_clique);
                    tabu_list_.RestrictRemovedVertex(vertex_clique);

                    AddToClique(non_neighbour_a);
                    AddToClique(non_neighbour_b);
                    tabu_list_.RestrictAddedVertex(non_neighbour_a);
                    tabu_list_.RestrictAddedVertex(non_neighbour_b);
                    return true;
                }
            }
        }

        return false;
    }

    bool Swap1To1() {
        for (int32_t index_clique = 0; index_clique <= index_q_; index_clique++) {
            int vertex_clique = qco_[index_clique];

            // We should not remove recently added vertex.
            if (tabu_list_.IsInAddedList(vertex_clique)) {
                continue;
            }

            for (const auto& non_neighbour: vertices_non_neighbours_[vertex_clique]) {
                // We should not add recently removed vertex.
                if (tabu_list_.IsInRemovedList(non_neighbour)) {
                    continue;
                }

                if (tightness_[non_neighbour] == 1) {
                    RemoveFromClique(vertex_clique);
                    tabu_list_.RestrictRemovedVertex(vertex_clique);

                    AddToClique(non_neighbour);
                    tabu_list_.RestrictAddedVertex(non_neighbour);
                    return true;
                }
            }
        }
        return false;
    }

    bool Move() {
        if (!HasCandidates()) {
            return false;
        }

        // todo(st235): Consider taking a random vertex.
        int vertex = qco_[index_c_];
        AddToClique(vertex);
        return true;
    }

    [[nodiscard]] inline std::unordered_set<int32_t> GetClique() const {
        std::unordered_set<int32_t> clique;
        for (int32_t i = 0; i <= index_q_; i++) {
            clique.insert(qco_[i]);
        }
        return std::move(clique);
    }

    [[nodiscard]] inline size_t CliqueSize() const {
        size_t clique_size = static_cast<size_t>(index_q_) + 1;
        assert(clique_size <= size_);
        return clique_size;
    }

    ~Clique() = default;
};

} // namespace

class MaxCliqueTabuSearch {
private:
    vector <unordered_set<int>> graph_;
    unordered_set<int> best_clique_;

    void RunInitialHeuristic(int randomization,
                             Clique& clique) {
        //todo(st235): replace initial heuristic.
        std::mt19937 generator;

        vector<int> candidates(graph_.size());
        for (size_t i = 0; i < graph_.size(); ++i) {
            candidates[i] = i;
        }
        shuffle(candidates.begin(), candidates.end(), generator);

        while (!candidates.empty()) {
            int last = candidates.size() - 1;
            int rnd = GenerateInRange(0, min(randomization - 1, last));
            int vertex = candidates[rnd];

            clique.AddToClique(vertex);

            for (int c = 0; c < candidates.size(); c++) {
                int candidate = candidates[c];
                if (graph_[vertex].find(candidate) == graph_[vertex].end()) {
                    // Move the candidate to the end and pop it
                    std::swap(candidates[c], candidates[candidates.size() - 1]);
                    candidates.pop_back();
                    --c;
                }
            }
            shuffle(candidates.begin(), candidates.end(), generator);
        }
    }

public:
    void ReadGraphFile(const std::string& filename) {
        std::ifstream fin(filename);
        std::string line;
        int vertices = 0, edges = 0;
        while (std::getline(fin, line)) {
            if (line[0] == 'c') {
                continue;
            }

            std::stringstream line_input(line);
            char command;
            if (line[0] == 'p') {
                std::string type;
                line_input >> command >> type >> vertices >> edges;
                graph_.resize(vertices);
            } else {
                int start, finish;
                line_input >> command >> start >> finish;
                // Edges in DIMACS file can be repeated, but it is not a problem for our sets
                graph_[start - 1].insert(finish - 1);
                graph_[finish - 1].insert(start - 1);
            }
        }
    }

    void RunSearch(int starts, int randomization) {
        for (int iter = 0; iter < starts; ++iter) {
            Clique clique(graph_.size(), graph_);
            RunInitialHeuristic(randomization, clique);

            int swaps = 0;
            while (swaps < 100) {
                if (!clique.Move()) {
                    if (!clique.Swap1To1()) {
                        break;
                    } else {
                        ++swaps;
                    }
                }
            }

            if (clique.CliqueSize() > best_clique_.size()) {
                best_clique_ = std::move(clique.GetClique());
            }
        }
    }

    const unordered_set<int>& GetClique() {
        return best_clique_;
    }

    bool Check() {
        for (int i: best_clique_) {
            for (int j: best_clique_) {
                if (i != j && graph_[i].count(j) == 0) {
                    cout << "Returned subgraph is not clique\n";
                    return false;
                }
            }
        }
        return true;
    }
};

int main() {
    int iterations;
    cout << "Number of iterations: ";
    cin >> iterations;
    int randomization;
    cout << "Randomization: ";
    cin >> randomization;

    vector<string> files = {
            "brock200_1.clq", "brock200_2.clq", "brock200_3.clq", "brock200_4.clq",
            "brock400_1.clq", "brock400_2.clq", "brock400_3.clq", "brock400_4.clq",
            "C125.9.clq",
            "gen200_p0.9_44.clq", "gen200_p0.9_55.clq",
            "hamming8-4.clq",
            "johnson16-2-4.clq", "johnson8-2-4.clq",
            "keller4.clq",
            "MANN_a27.clq", "MANN_a9.clq",
            "p_hat1000-1.clq", "p_hat1000-2.clq", "p_hat1500-1.clq",
            "p_hat300-3.clq", "p_hat500-3.clq",
            "san1000.clq",
            "sanr200_0.9.clq", "sanr400_0.7.clq" };

    std::ofstream fout("clique_tabu.csv");
    fout << "File; Clique; Time (sec)\n";

    std::cout << std::setfill(' ') << std::setw(20) << "Instance"
              << std::setfill(' ') << std::setw(10) << "Clique"
              << std::setfill(' ') << std::setw(15) << "Time, sec"
              << std::endl;

    for (const auto& file: files) {
        MaxCliqueTabuSearch problem;
        problem.ReadGraphFile("data/" + file);

        clock_t start = clock();
        problem.RunSearch(iterations, randomization);

        clock_t end = clock();
        clock_t ticks_diff = end - start;
        double seconds_diff = RoundTo(double(ticks_diff) / CLOCKS_PER_SEC, 0.001);

        if (!problem.Check()) {
            cout << "*** WARNING: incorrect clique ***\n";
            fout << "*** WARNING: incorrect clique ***\n";
        }

        fout << file << "; " << problem.GetClique().size() << "; " << seconds_diff << '\n';

        std::cout << std::setfill(' ') << std::setw(20) << file
                  << std::setfill(' ') << std::setw(10) << problem.GetClique().size()
                  << std::setfill(' ') << std::setw(15) << seconds_diff
                  << std::endl;
    }

    fout.close();
    return 0;
}
