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

static Whelk whelks[12427675];  // hack to defer construction costs,
                                // this is more than the number of iterations
                                // that should usually be run.

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

Clam consumeByConstRef(Clam const& w) {
  Clam copy(w);
  return copy;
}

Clam consumeByValue(Clam w) {
  Clam copy(std::move(w));
  return copy;
}

Clam consumeByRvalueRef(Clam&& w) {
  Clam copy(std::move(w));
  return copy;
}

static void ClamConstRef(benchmark::State& state) {
  Clam w;
  w.set_name("Steve");
  w.set_id(6);
  w.set_genus("Tridacna");
  w.set_spawned(true);
  w.set_fluted_description("could be more fluted, could be less fluted");
  w.set_mass_kg(40);
  w.set_has_pearl(false);
  w.set_pearl_mass(0);
  for(int i = 0; i < 30; ++i) {
    w.add_spawning_phases("At a quarter moon");
  }
  for(auto _ : state ){
    consumeByConstRef(w);
  }
}

static Clam clams[331280];  //  to defer construction costs,
                            // this is more than the number of iterations
                            // that should usually be run.

static void ClamRvalueRef(benchmark::State& state) {
  auto clam_ptr = &(clams[0]);

  for(auto& it : clams) {
    it.set_name("Steve");
    it.set_id(6);
    it.set_genus("Tridacna");
    it.set_spawned(true);
    it.set_fluted_description("could be more fluted, could be less fluted");
    it.set_mass_kg(40);
    it.set_has_pearl(false);
    for(int i = 0; i < 30; ++i) {
      it.add_spawning_phases("At a quarter moon");
    }
    it.set_pearl_mass(0);
  }

  for(auto _ : state ){
    consumeByRvalueRef(std::move(*clam_ptr));
    clam_ptr++;
  }
}

static void ClamValue(benchmark::State& state) {
  Clam w;
  w.set_name("Steve");
  w.set_id(6);
  w.set_genus("Tridacna");
  w.set_spawned(true);
  w.set_fluted_description("could be more fluted, could be less fluted");
  w.set_mass_kg(40);
  w.set_has_pearl(false);
  w.set_pearl_mass(0);
  for(int i = 0; i < 30; ++i) {
    w.add_spawning_phases("At a quarter moon");
  }

  for(auto _ : state ){
    consumeByValue(w);
  }
}

BENCHMARK(ClamConstRef);
BENCHMARK(ClamRvalueRef);
BENCHMARK(ClamValue);
