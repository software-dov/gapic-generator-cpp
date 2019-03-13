// Copyright 2019 Google Inc.  All rights reserved
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "benchmark/benchmark.h"
#include "gax/molluscs.pb.h"

#include <vector>

using namespace molluscs;

Whelk consumeByConstRef(Whelk const& w) {
  Whelk copy(w);
  return copy;
}

Whelk consumeByValue(Whelk w) {
  Whelk copy(std::move(w));
  return copy;
}

Whelk consumeByRvalueRef(Whelk&& w) {
  Whelk copy(std::move(w));
  return copy;
}

static void WhelkConstRef(benchmark::State& state) {
  Whelk w;
  w.set_name("Steve");
  w.set_id(6);
  for(auto _ : state ){
    consumeByConstRef(w);
  }
}

static Whelk whelks[12427675];

static void WhelkRvalueRef(benchmark::State& state) {
  auto whelk_ptr = &(whelks[0]);

  for(auto& it : whelks) {
    it.set_name("Steve");
    it.set_id(6);
  }

  for(auto _ : state ){
    consumeByRvalueRef(std::move(*whelk_ptr));
    whelk_ptr++;
  }
}

static void WhelkValue(benchmark::State& state) {
  Whelk w;
  w.set_name("Steve");
  w.set_id(6);
  for(auto _ : state ){
    consumeByValue(w);
  }
}

BENCHMARK(WhelkConstRef);
BENCHMARK(WhelkRvalueRef);
BENCHMARK(WhelkValue);
