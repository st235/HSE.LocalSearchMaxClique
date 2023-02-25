#include <iostream>
#include <unordered_map>

#define REQUIRE_TRUE(x) { if (!(x)) { std::cerr << __FUNCTION__ << " was false,\nbut expected true\non line: " << __LINE__ << std::endl; } else { std::cout << __FUNCTION__ << " passed" << std::endl; } }
#define REQUIRE_FALSE(x) { if ((x)) { std::cerr << __FUNCTION__ << " was true,\nbut expected false\non line: " << __LINE__ << std::endl; } else { std::cout << __FUNCTION__ << " passed" << std::endl; } }

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

namespace tests {

void constructor_createsEmptyList() {
    std::linked_unordered_set<int32_t> list(10);
    REQUIRE_TRUE(list.empty())
}

void copyConstructor_copiesContentDeeply() {
    std::linked_unordered_set<int32_t> a(10);

    a.insert(5);
    a.insert(6);
    a.insert(7);

    std::linked_unordered_set<int32_t> b(a);

    a.remove(6);
    b.insert(8);

    REQUIRE_TRUE(a.size() == 2)
    REQUIRE_TRUE(b.size() == 4)
}


void insert_withinCapacity_increaseSize() {
    std::linked_unordered_set<int32_t> list(2);

    list.insert(5);
    REQUIRE_TRUE(list.size() == 1)
}

void insert_overCapacity_doNotIncreaseSize() {
    std::linked_unordered_set<int32_t> list(2);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_TRUE(list.size() == 2)
}

void insert_duplicateValue_doNotChangeSize() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    list.insert(5);
    list.insert(5);

    REQUIRE_TRUE(list.size() == 3)
}

void contains_itemNotFromList_returnsFalse() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_FALSE(list.contains(2))
    REQUIRE_FALSE(list.contains(4))
    REQUIRE_FALSE(list.contains(6))
}

void contains_itemFromList_returnsTrue() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_TRUE(list.contains(1))
    REQUIRE_TRUE(list.contains(3))
    REQUIRE_TRUE(list.contains(5))
}

void remove_itemNotFromList_returnsFalseAndDoNotChangeSize() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_FALSE(list.remove(2))
    REQUIRE_TRUE(list.size() == 3)
}

void remove_itemFromList_returnsTrueDecreasesSize() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_TRUE(list.remove(1))
    REQUIRE_TRUE(list.size() == 2)

    REQUIRE_TRUE(list.remove(3))
    REQUIRE_TRUE(list.size() == 1)
}

void remove_theSameItemTwice_returnsFalseSecondTime() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_TRUE(list.remove(1))
    REQUIRE_TRUE(list.size() == 2)

    REQUIRE_FALSE(list.remove(1))
    REQUIRE_TRUE(list.size() == 2)
}

void removeLast_removesLastItem() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    REQUIRE_TRUE(list.remove() == 5)
    REQUIRE_TRUE(list.size() == 2)
}

void insert_theSameItem_changesItemRemoveOrder() {
    std::linked_unordered_set<int32_t> list(10);

    list.insert(5);
    list.insert(3);
    list.insert(1);

    list.insert(5);

    REQUIRE_TRUE(list.remove() == 3)
    REQUIRE_TRUE(list.size() == 2)
}

}

int main() {
    tests::constructor_createsEmptyList();
    tests::copyConstructor_copiesContentDeeply();

    tests::insert_withinCapacity_increaseSize();
    tests::insert_overCapacity_doNotIncreaseSize();
    tests::insert_duplicateValue_doNotChangeSize();

    tests::contains_itemNotFromList_returnsFalse();
    tests::contains_itemFromList_returnsTrue();

    tests::remove_itemNotFromList_returnsFalseAndDoNotChangeSize();
    tests::remove_itemFromList_returnsTrueDecreasesSize();
    tests::remove_theSameItemTwice_returnsFalseSecondTime();

    tests::removeLast_removesLastItem();
    tests::insert_theSameItem_changesItemRemoveOrder();

    return 0;
}