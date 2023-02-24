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
#include <unordered_set>
#include <algorithm>

using namespace std;

namespace {

int32_t GenerateInRange(int32_t start, int32_t finish) {
    int32_t width = finish - start + 1;
    return static_cast<int32_t>(std::rand() % width + start);
}

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
        tightness_(size) {
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

    bool Swap1To1() {
        for (int32_t index_clique = 0; index_clique <= index_q_; index_clique++) {
            int vertex_clique = qco_[index_clique];

            for (const auto& non_neighbour: vertices_non_neighbours_[vertex_clique]) {
                if (tightness_[non_neighbour] == 1) {
                    RemoveFromClique(vertex_clique);
                    AddToClique(non_neighbour);
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

    const unordered_set<int> &GetClique() {
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
            "brock200_1.clq", "brock200_2.clq", "brock200_3.clq", "brock200_4.clq",
            "C125.9.clq",
            "gen200_p0.9_44.clq", "gen200_p0.9_55.clq",
            "hamming8-4.clq",
            "johnson16-2-4.clq", "johnson8-2-4.clq",
            "keller4.clq",
            "MANN_a27.clq", "MANN_a9.clq",
            "p_hat1000-1.clq", "p_hat1000-2.clq", "p_hat1500-1.clq",
            "p_hat300-3.clq", "p_hat500-3.clq",
            "san1000.clq",
            "sanr200_0.9.clq", "sanr400_0.7.clq"};

    ofstream fout("clique_tabu.csv");
    fout << "File; Clique; Time (sec)\n";
    for (string file: files) {
        MaxCliqueTabuSearch problem;
        problem.ReadGraphFile("data/" + file);
        clock_t start = clock();
        problem.RunSearch(iterations, randomization);
        if (!problem.Check()) {
            cout << "*** WARNING: incorrect clique ***\n";
            fout << "*** WARNING: incorrect clique ***\n";
        }
        fout << file << "; " << problem.GetClique().size() << "; " << double(clock() - start) / CLOCKS_PER_SEC << '\n';
        cout << file << ", result - " << problem.GetClique().size() << ", time - "
             << double(clock() - start) / CLOCKS_PER_SEC << '\n';
    }

    fout.close();
    return 0;
}
