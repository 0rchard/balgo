/*
 * Copyright (c) 2013 Binson Zhang.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @author	Binson Zhang <bin183cs@gmail.com>
 * @date		2013-8-18
 */

#ifndef BALGO_AC_AC_DA_TRIE_H_
#define BALGO_AC_AC_DA_TRIE_H_

#include <stdint.h>
#include <algorithm>
#include <string>
#include <vector>

#include "balgo/trie/trie_traits.h"
#include "ac_trie.h"

namespace balgo {

/**
 * @brief Double-array Trie for AC
 */
template<typename Char = char, typename Value = uint32_t, typename NodePtr = uint32_t>
class AcDaTrie : public AcTrie<Char, Value, NodePtr> {
 public:
  typedef typename TrieTraits<Char>::UChar UChar;

  using AcTrie<Char, Value, NodePtr>::Null;

  struct Node;
  struct AuxUnit;
  struct Key;
  typedef std::vector<Node> NodeContainer;
  typedef std::vector<AuxUnit> AuxContainer;
  typedef std::vector<Key> KeyContainer;
  typedef std::vector<NodePtr> KidContainer;
  typedef std::vector<Value> ValueContainer;

  struct Node {
    NodePtr base;
    NodePtr check;
    NodePtr fail;
    NodePtr report;
    Char label;
    Char child_label;
    UChar sibling;
    bool final;
    Node()
        : base(Null()),
          check(Null()),
          fail(Null()),
          report(Null()),
          label(NullChar()),
          child_label(NullChar()),
          sibling(0),
          final(false) {
    }

    NodePtr GetValueIndex() const {
      return base;
    }

    void SetValueIndex(NodePtr idx) {
      base = idx;
    }

    std::string ToString() const {
      std::stringstream ss;
      ss << this << "(base=" << base << ", check=" << check << ", fail=" << fail
          << ", report=" << report << ", label=" << label
          << ", child_label=" << child_label << ", sibling="
          << ToUInt32(sibling) << ", final=" << final << ")";
      return ss.str();
    }

   private:
    static uint32_t ToUInt32(UChar c) {
      return static_cast<uint32_t>(c);
    }

    static uint32_t ToUInt32(Char c) {
      return ToUInt32(static_cast<UChar>(c));
    }
  };

  struct Key {
    const Char* begin;
    std::size_t length;
    Key(const Char* beg, std::size_t len)
        : begin(beg),
          length(len) {
    }
    bool operator<(const Key& rhs) const {
      std::size_t minlen = std::min(length, rhs.length);
      for (std::size_t i = 0; i < minlen; ++i) {
        if (Index(begin[i]) < Index(rhs.begin[i])) {
          return true;
        } else if (Index(begin[i]) > Index(rhs.begin[i])) {
          return false;
        }
      }
      return length < rhs.length;
    }
    bool operator==(const Key& rhs) const {
      std::size_t minlen = std::min(length, rhs.length);
      for (std::size_t i = 0; i < minlen; ++i) {
        if (Index(begin[i]) != Index(rhs.begin[i])) {
          return false;
        }
      }
      return length == rhs.length;
    }
  };

  class KeyIdLess {
   public:
    explicit KeyIdLess(KeyContainer& keys)
        : keys_(keys) {
    }
    bool operator()(const NodePtr lhs, const NodePtr rhs) const {
      return keys_[lhs] < keys_[rhs];
    }
   private:
    KeyContainer& keys_;
  };

  class KeyIdEqual {
   public:
    explicit KeyIdEqual(KeyContainer& keys)
        : keys_(keys) {
    }
    bool operator()(const NodePtr lhs, const NodePtr rhs) const {
      return keys_[lhs] == keys_[rhs];
    }
   private:
    KeyContainer& keys_;
  };

  struct AuxUnit {
    NodePtr prev;
    NodePtr next;
    bool used;
    AuxUnit()
        : prev(Null()),
          next(Null()),
          used(false) {
    }
  };

  AcDaTrie() {
  }
  virtual ~AcDaTrie() {
  }

  virtual NodePtr Root() const {
    return 1;
  }

  NodePtr Child(const NodePtr parent, Char label) const {
    NodePtr child = units_[parent].base + Index(label);
    if (child < units_.size() && units_[child].check == parent) {
      return child;
    }
    return Null();
  }

  virtual bool IsFinal(const NodePtr node) const {
    return units_[units_[node].base].check == node;
  }

  virtual const Value* GetValue(NodePtr p) const {
    if (IsFinal(p)) {
      NodePtr kid = units_[units_[p].base].GetValueIndex();
      return &(values_[kid]);
    }
    return NULL;
  }

  NodePtr FirstChild(const NodePtr parent) const {
    if (units_[parent].child_label != NullChar()) {
      return units_[parent].base + Index(units_[parent].child_label);
    }
    return Null();
  }

  NodePtr Sibling(const NodePtr node) const {
    if (units_[node].sibling == 0) {
      return Null();
    }
    return node + units_[node].sibling;
  }

  NodePtr Fail(const NodePtr node) const {
    return units_[node].fail;
  }

  void SetFail(NodePtr node, const NodePtr fail) {
    units_[node].fail = fail;
  }

  bool Final(const NodePtr node) const {
    return units_[node].final;
  }

  NodePtr Report(const NodePtr node) const {
    return units_[node].report;
  }

  void SetReport(const NodePtr node, NodePtr report) {
    units_[node].report = report;
  }

  Char Label(const NodePtr node) const {
    return units_[node].label;
  }

  virtual void Insert(const Char* begin, const Char* end, const Value &value) {
    if (begin != end) {
      std::size_t length = static_cast<std::size_t>(std::distance(begin, end));
      keys_.push_back(Key(begin, length));
      values_.push_back(value);
    }
  }

  virtual void Build(bool sort = true) {
    kids_.clear();
    for (std::size_t i = 0; i < keys_.size(); ++i) {
      kids_.push_back(i);
    }
    if (sort) {
      std::sort(kids_.begin(), kids_.end(), KeyIdLess(keys_));
      typename KidContainer::iterator new_end = std::unique(kids_.begin(), kids_.end(),
                                                            KeyIdEqual(keys_));
      std::size_t new_size = static_cast<std::size_t>(std::distance(kids_.begin(), new_end));
      kids_.resize(new_size);
    }
    // init auxes_
    static const NodePtr kFreeHead = (1 << (sizeof(UChar) * 8)) + Root();
    free_head_ = kFreeHead;
    units_.resize(free_head_ + 1);
    auxes_.resize(free_head_ + 1);
    auxes_[free_head_].prev = free_head_;
    auxes_[free_head_].next = free_head_;

    Unit(Root()).check = Root();

    BuildNode(0, Root(), 0, kids_.size());

    // release auxes_
    KeyContainer().swap(keys_);
    AuxContainer().swap(auxes_);
  }

  virtual void Clear() {
    units_.clear();
    values_.clear();
    kids_.clear();
    keys_.clear();
    auxes_.clear();
  }

  virtual std::size_t NodeSize() const {
    return sizeof(Node);
  }

  virtual std::size_t NumNodes() const {
    return units_.size();
  }

  virtual std::string Name() const {
    return "DaTrie";
  }

  std::string ToString() const {
    std::stringstream ss;
    for (std::size_t i = Root(); i < units_.size(); ++i) {
      if (units_[i].base != Null()) {
        ss << "[" << i << "] " << units_[i].ToString() << "\n";
      }
    }
    return ss.str();
  }

 private:
  static Char NullChar() {
    return 0;
  }

  static UChar Index(Char label) {
    return static_cast<UChar>(label);
  }

  Node& Unit(const NodePtr index) {
    if (index >= units_.size()) {
      Resize(index + 1);
    }
    return units_[index];
  }

  void Resize(std::size_t size) {
    if (size <= units_.size())
      return;

    units_.resize(size);
    std::size_t old_size = auxes_.size();
    auxes_.resize(size);
    for (std::size_t i = old_size + 1; i < auxes_.size(); ++i) {
      auxes_[i - 1].next = i;
      auxes_[i].prev = i - 1;
    }
    NodePtr old_tail = auxes_[free_head_].prev;
    NodePtr new_tail = size - 1;
    auxes_[old_tail].next = old_size;
    auxes_[old_size].prev = old_tail;
    auxes_[free_head_].prev = new_tail;
    auxes_[new_tail].next = free_head_;
  }

  void Reserve(NodePtr index) {
    AuxUnit& aux = auxes_[index];
    aux.used = true;
    auxes_[aux.prev].next = aux.next;
    auxes_[aux.next].prev = aux.prev;
  }

  Char Label(NodePtr pid, std::size_t depth) {
    if (depth < keys_[pid].length) {
      return keys_[pid].begin[depth];
    }
    return NullChar();
  }

  void BuildNode(std::size_t depth, NodePtr parent, NodePtr begin, NodePtr end) {
    if (begin == end)
      return;

    std::vector<Char> labels;
    std::vector<NodePtr> guards;
    for (NodePtr i = begin; i < end; ++i) {
      Char label = Label(kids_[i], depth);
      if (!labels.size() || labels.back() != label) {
        labels.push_back(label);
        guards.push_back(i);
      }
    }
    guards.push_back(end);

    NodePtr base = Fetch(labels);
    units_[parent].base = base;
    InsertUnits(parent, base, labels);
    if (labels[0] != NullChar()) {
      units_[parent].child_label = labels[0];
    } else {
      units_[parent].final = true;
      NodePtr child = base + Index(labels[0]);
      units_[child].SetValueIndex(kids_[begin]);  // store key id
      if (labels.size() > 1)
      {
        units_[parent].child_label = labels[1];
      }
    }

    for (std::size_t i = units_[parent].final; i < labels.size(); ++i) {
      NodePtr child = base + Index(labels[i]);
      BuildNode(depth + 1, child, guards[i], guards[i + 1]);
    }
  }

  NodePtr Fetch(const std::vector<Char>& labels) {
    // Start at next of free_head_, so free_head_ will never be used
    NodePtr free_idx = auxes_[free_head_].next;
    while (free_idx != free_head_) {
      NodePtr base = free_idx - Index(labels[0]);
      bool fetched = true;
      for (std::size_t i = 0; i < labels.size(); ++i) {
        NodePtr p = base + Index(labels[i]);
        if (p >= NumNodes()) {
          break;
        } else if (auxes_[p].used) {
          fetched = false;
          break;
        }
      }
      if (fetched) {
        return base;
      }
      free_idx = auxes_[free_idx].next;
    }
    return NumNodes() - Index(labels[0]);
  }

  void InsertUnits(NodePtr parent, NodePtr base, const std::vector<Char>& labels) {
    if (!labels.size())
    return;
    NodePtr max_idx = base + Index(labels.back());
    Resize(max_idx + 1);
    for (std::size_t i = 0; i < labels.size(); ++i) {
      NodePtr idx = base + Index(labels[i]);
      Node& unit = Unit(idx);
      Reserve(idx);
      unit.check = parent;
      unit.label = labels[i];
      if (i + 1 < labels.size()) {
        unit.sibling = Index(labels[i+1]) - Index(labels[i]);
      } else {
        unit.sibling = 0;
      }
    }
  }

  std::string NodeString(NodePtr node) const {
    return units_[node].ToString();
  }

  NodeContainer units_;
  ValueContainer values_;
  KidContainer kids_;

  KeyContainer keys_;
  NodePtr free_head_;
  AuxContainer auxes_;
};

}  // namespace balgo
#endif  // BALGO_AC_AC_DA_TRIE_H_
