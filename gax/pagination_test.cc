// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gax/pagination.h"
#include "google/longrunning/operations.pb.h"
#include "gax/status.h"
#include <google/protobuf/util/message_differencer.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace ::google;

class OperationsAccessor {
 public:
  inline protobuf::RepeatedPtrField<longrunning::Operation>* operator()(
      longrunning::ListOperationsResponse& lor) const {
    return lor.mutable_operations();
  }
};

class PageRetriever {
 public:
  PageRetriever(int max_pages, int elts_per_page, int fail_after_page = 0)
      : i_(1),
        max_pages_(max_pages),
        elts_per_page_(elts_per_page),
        fail_after_page_(fail_after_page) {}
  gax::Status operator()(longrunning::ListOperationsResponse* lor) {
    if (i_ <= max_pages_) {
      std::stringstream ss;
      ss << "NextPage" << i_;
      lor->set_next_page_token(ss.str());
      for (int j = 0; j < elts_per_page_; j++) {
        std::stringstream elt_name;
        elt_name << "Element " << i_ << "x" << j;
        auto op = lor->add_operations();
        op->set_name(elt_name.str());
      }
      i_++;
    } else {
      lor->Clear();
    }

    return gax::Status{};
  }

 private:
  int i_;
  const int max_pages_;
  const int elts_per_page_;
  const int fail_after_page_;
};

using TestPages =
    gax::Pages<longrunning::Operation, longrunning::ListOperationsResponse,
               OperationsAccessor, PageRetriever>;

using TestedPageResult = TestPages::PageResult;

TestedPageResult MakeTestedPageResult() {
  longrunning::ListOperationsResponse response;
  response.set_next_page_token("NextPage");

  for (int i = 0; i < 10; i++) {
    std::stringstream ss;
    ss << "TestOperation" << i;
    auto operation = response.add_operations();
    operation->set_name(ss.str());
  }

  return TestedPageResult(std::move(response));
}

TEST(PageResult, RawPage) {
  TestedPageResult page_result = MakeTestedPageResult();

  EXPECT_EQ(page_result.NextPageToken(), "NextPage");
  EXPECT_EQ(page_result.NextPageToken(),
            page_result.RawPage().next_page_token());
  EXPECT_EQ(page_result.RawPage().operations_size(), 10);
}

TEST(PageResult, Accessors) {
  TestedPageResult page_result = MakeTestedPageResult();
  EXPECT_EQ((*page_result.begin()).name(), "TestOperation0");
  EXPECT_EQ(page_result.begin()->name(), "TestOperation0");
}

TEST(PageResult, BasicIteration) {
  TestedPageResult page_result = MakeTestedPageResult();
  auto prIt = page_result.begin();
  auto reIt = page_result.RawPage().operations().begin();
  for (; prIt != page_result.end() &&
         reIt != page_result.RawPage().operations().end();
       ++prIt, ++reIt) {
    // Note: cannot use EXPECT_EQ for the elements or on vectors constructed
    // from the respective iterators because messages do not define operator==
    // as a member function.
    EXPECT_TRUE(protobuf::util::MessageDifferencer::Equals(*prIt, *reIt));
  }
  EXPECT_EQ(prIt, page_result.end());
  EXPECT_EQ(reIt, page_result.RawPage().operations().end());
}

TEST(PageResult, MoveIteration) {
  TestedPageResult page_result = MakeTestedPageResult();
  std::vector<longrunning::Operation> ops{
      std::move_iterator<TestedPageResult::iterator>(page_result.begin()),
      std::move_iterator<TestedPageResult::iterator>(page_result.end())};
  EXPECT_EQ(page_result.begin()->name(), "");
}

TEST(Pages, Basic) {
  TestPages terminal(
      // The output param is pristine, which means its next_page_token
      // is empty.
      PageRetriever(0, 0), 0);

  EXPECT_EQ(terminal.begin(), terminal.end());
  EXPECT_EQ(terminal.end()->NextPageToken(), "");
}

TEST(Pages, Iteration) {
  int i = 1;
  TestPages pages(PageRetriever(10, 0), 0);
  auto iter = pages.begin();
  for (; iter != pages.end(); ++iter) {
    std::stringstream ss;
    ss << "NextPage" << i;

    // Sanity check on both access operators
    EXPECT_EQ(iter->NextPageToken(), ss.str());
    EXPECT_EQ((*iter).NextPageToken(), ss.str());
    i++;
  }
  EXPECT_EQ(i, 11);
  EXPECT_EQ(iter->NextPageToken(), "");
}

// TEST(Pages, MoveIteration) {
//   TestPages pages(PageRetriever(10, 0), 0);
//   std::vector<longrunning::ListOperationsResponse> lor{
//       std::move_iterator<TestPages::iterator>(pages.begin()),
//       std::move_iterator<TestPages::iterator>(pages.end())};
// }

TEST(Pages, PageCap) {
  int i = 1;
  TestPages pages(PageRetriever(10, 0), 5);
  auto iter = pages.begin();
  for (; iter != pages.end(); ++iter) {
    std::stringstream ss;
    ss << "NextPage" << i;

    EXPECT_EQ(iter->NextPageToken(), ss.str());
    i++;
  }
  EXPECT_EQ(i, 5);
  EXPECT_EQ(iter->NextPageToken(), "NextPage5");
}

TEST(Pages, MultipleIteration) {
  TestPages pages(PageRetriever(5, 0), 0);
  std::vector<std::string> nextTokens1;
  nextTokens1.reserve(5);

  std::transform(pages.begin(), pages.end(), std::back_inserter(nextTokens1),
                 [](TestedPageResult const& p) -> std::string {
                   return p.NextPageToken();
                 });

  std::vector<std::string> nextTokens2;
  nextTokens2.reserve(5);
  std::transform(pages.begin(), pages.end(), std::back_inserter(nextTokens2),
                 [](TestedPageResult const& p) -> std::string {
                   return p.NextPageToken();
                 });

  EXPECT_EQ(nextTokens1, nextTokens2);
}

using TestPaginatedResult =
    gax::PaginatedResult<longrunning::Operation,
                         longrunning::ListOperationsResponse,
                         OperationsAccessor, PageRetriever>;

std::vector<std::string> make_expected_names(int pages, int elts) {
  std::vector<std::string> expected_names;
  expected_names.reserve(pages * elts);
  for (int i = 1; i < pages + 1; i++) {
    for (int j = 0; j < elts; j++) {
      std::stringstream elt_name;
      elt_name << "Element " << i << "x" << j;
      expected_names.push_back(elt_name.str());
    }
  }
  return expected_names;
}

TEST(PaginatedResult, BasicIteration) {
  std::vector<std::string> expected_names = make_expected_names(5, 5);

  std::vector<std::string> names;
  names.reserve(expected_names.size());
  TestPaginatedResult paginated_result(PageRetriever(5, 5), 0);
  for (auto& e : paginated_result) {
    names.push_back(e.name());
  }
  EXPECT_EQ(expected_names, names);
}

// TEST(PaginatedResult, MoveIteration) {
//   TestPaginatedResult paginated_result(PageRetriever(5, 5), 0);
//   std::vector<longrunning::Operation> ops{
//       std::move_iterator<TestPaginatedResult::iterator>(
//           paginated_result.begin()),
//       std::move_iterator<TestPaginatedResult::iterator>(paginated_result.end()),
//   };
// }

TEST(PaginatedResult, CappedIteration) {
  std::vector<std::string> expected_names = make_expected_names(3, 5);
  std::vector<std::string> names;
  names.reserve(expected_names.size());
  TestPaginatedResult paginated_result(PageRetriever(5, 5), 4);
  std::transform(paginated_result.begin(), paginated_result.end(),
                 std::back_inserter(names),
                 [](longrunning::Operation const& op) -> std::string {
                   return op.name();
                 });
  EXPECT_EQ(expected_names, names);
}

TEST(PaginatedResult, Pages) {
  std::vector<std::string> expected_names = make_expected_names(5, 5);

  std::vector<std::string> names;
  names.reserve(expected_names.size());
  TestPaginatedResult paginated_result(PageRetriever(5, 5), 0);
  for (auto& p : paginated_result.pages()) {
    for (auto& e : p) {
      names.push_back(e.name());
    }
  }

  EXPECT_EQ(expected_names, names);
}

TEST(PaginatedResult, CappedPages) {
  std::vector<std::string> expected_names = make_expected_names(3, 5);

  std::vector<std::string> names;
  names.reserve(expected_names.size());
  TestPaginatedResult paginated_result(PageRetriever(10, 5), 4);
  for (auto& p : paginated_result.pages()) {
    for (auto& e : p) {
      names.push_back(e.name());
    }
  }

  EXPECT_EQ(expected_names, names);
}

TEST(PaginatedResult, MultipleIteration) {
  TestPaginatedResult paginated_result(PageRetriever(5, 5), 0);

  std::vector<std::string> names1;
  names1.reserve(25);
  std::transform(paginated_result.begin(), paginated_result.end(),
                 std::back_inserter(names1),
                 [](longrunning::Operation const& op) -> std::string {
                   return op.name();
                 });

  std::vector<std::string> names2;
  names2.reserve(names1.size());

  std::transform(paginated_result.begin(), paginated_result.end(),
                 std::back_inserter(names2),
                 [](longrunning::Operation const& op) -> std::string {
                   return op.name();
                 });

  EXPECT_EQ(names1, names2);
}

}  // namespace
